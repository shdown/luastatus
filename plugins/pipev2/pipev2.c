/*
 * Copyright (C) 2015-2026  luastatus developers
 *
 * This file is part of luastatus.
 *
 * luastatus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * luastatus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with luastatus.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <lua.h>
#include <lauxlib.h>

#include "libls/ls_algo.h"
#include "libls/ls_alloc_utils.h"
#include "libls/ls_panic.h"
#include "libls/ls_io_utils.h"
#include "libls/ls_tls_ebuf.h"
#include "libls/ls_time_utils.h"

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "launch.h"
#include "sigdb.h"
#include "nb_writer.h"
#include "nb_reader.h"

enum {
    PID_NOT_SPAWNED_YET = 0,
    PID_ALREADY_TERMINATED = -1,
};

typedef struct {
    pthread_mutex_t mtx;
    int stdin_fd;
    NB_Writer *stdin_writer;
    pid_t pid;
} ChildState;

static void child_state_new(ChildState *x)
{
    LS_PTH_CHECK(pthread_mutex_init(&x->mtx, NULL));
    x->stdin_fd = -1;
    x->stdin_writer = NULL;
    x->pid = PID_NOT_SPAWNED_YET;
}

static void child_state_destroy(ChildState *x)
{
    LS_PTH_CHECK(pthread_mutex_destroy(&x->mtx));
    ls_close(x->stdin_fd);
    if (x->stdin_writer) {
        nb_writer_destroy(x->stdin_writer);
    }
}

typedef struct {
    LaunchArg *data;
    size_t size;
    size_t capacity;
} ArgsList;

typedef struct {
    ArgsList argv;
    char *file_path;
    char delimiter;

    bool pipe_stdin;
    bool greet;
    bool bye;
    bool report_non_whole;

    ChildState child_state;
} Priv;

static void args_list_add(ArgsList *x, const char *s)
{
    char *new_elem = s ? ls_xstrdup(s) : NULL;

    if (x->size == x->capacity) {
        x->data = LS_M_X2REALLOC(x->data, &x->capacity);
    }
    x->data[x->size++] = new_elem;
}

static void args_list_destroy(ArgsList *x)
{
    for (size_t i = 0; i < x->size; ++i) {
        free(x->data[i]);
    }
    free(x->data);
}

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;

    args_list_destroy(&p->argv);

    free(p->file_path);

    child_state_destroy(&p->child_state);

    free(p);
}

static int visit_argv_elem(MoonVisit *mv, void *ud, int kpos, int vpos)
{
    (void) kpos;
    mv->where = "'argv' element";
    Priv *p = ud;

    if (moon_visit_checktype_at(mv, NULL, vpos, LUA_TSTRING) < 0) {
        return -1;
    }
    const char *s = lua_tostring(mv->L, vpos);
    args_list_add(&p->argv, s);
    return 0;
}

static int visit_delimiter(MoonVisit *mv, void *ud, const char *s, size_t ns)
{
    Priv *p = ud;

    if (ns != 1) {
        moon_visit_err(mv, "length of delimiter: expected 1, found %zu", ns);
        return -1;
    }

    p->delimiter = s[0];
    return 0;
}

static int init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .argv = {0},
        .file_path = NULL,
        .delimiter = '\n',
        .pipe_stdin = false,
        .greet = false,
        .bye = false,
        .report_non_whole = false,
    };
    child_state_new(&p->child_state);

    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse argv
    if (moon_visit_table_f(&mv, -1, "argv", visit_argv_elem, p, true) < 0) {
        goto mverror;
    }
    // Parse file_path
    if (moon_visit_str(&mv, -1, "file_path", &p->file_path, NULL, true) < 0) {
        goto mverror;
    }

    if (!p->argv.size && !p->file_path) {
        snprintf(
            errbuf, sizeof(errbuf),
            "'argv' is empty or nil, and no 'file_path' specified");
        goto mverror;
    }
    if (p->argv.size && p->file_path) {
        snprintf(errbuf, sizeof(errbuf), "both 'argv' and 'file_path' were specified");
        goto mverror;
    }

    // Parse delimiter
    if (moon_visit_str_f(&mv, -1, "delimiter", visit_delimiter, p, true) < 0) {
        goto mverror;
    }

    // Parse pipe_stdin
    if (moon_visit_bool(&mv, -1, "pipe_stdin", &p->pipe_stdin, true) < 0) {
        goto mverror;
    }
    if (p->pipe_stdin && p->file_path) {
        snprintf(errbuf, sizeof(errbuf), "'pipe_stdin' with file mode does not make sense");
        goto mverror;
    }

    // Parse greet
    if (moon_visit_bool(&mv, -1, "greet", &p->greet, true) < 0) {
        goto mverror;
    }

    // Parse bye
    if (moon_visit_bool(&mv, -1, "bye", &p->bye, true) < 0) {
        goto mverror;
    }

    // Parse report_non_whole
    if (moon_visit_bool(&mv, -1, "report_non_whole", &p->report_non_whole, true) < 0) {
        goto mverror;
    }

    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
//error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static int l_write_to_stdin(lua_State *L) /*__THROWABLE__*/
{
    Priv *p = lua_touserdata(L, lua_upvalueindex(1));

    if (p->file_path) {
        return luaL_error(L, "file mode is used");
    }

    if (!p->pipe_stdin) {
        return luaL_error(L, "'pipe_stdin' option was not enabled");
    }

    size_t ndata;
    const char *data = luaL_checklstring(L, 1, &ndata);

    int rc;

    LS_PTH_CHECK(pthread_mutex_lock(&p->child_state.mtx));
    NB_Writer *writer = p->child_state.stdin_writer;
    if (writer) {
        rc = 1;
        nb_writer_enqueue(writer, data, ndata);
    } else {
        if (p->child_state.pid == PID_NOT_SPAWNED_YET) {
            rc = -1;
        } else {
            rc = 0;
        }
    }
    LS_PTH_CHECK(pthread_mutex_unlock(&p->child_state.mtx));

    if (rc < 0) {
        return luaL_error(L, "process has not been created yet");
    } else if (rc == 0) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, "child terminated");
        return 2;
    } else {
        lua_pushboolean(L, 1);
        return 1;
    }
}

static void fetch_sig_num(lua_State *L, int *out)
{
    if (lua_isnoneornil(L, 1)) {
        *out = SIGTERM;
        return;
    }

    int t = lua_type(L, 1);
    if (t == LUA_TNUMBER) {
        int res = lua_tointeger(L, 1);
        if (res < 0) {
            (void) luaL_argerror(L, 1, "number is negative or out of range");
            LS_MUST_BE_UNREACHABLE();
        }
        *out = res;

    } else if (t == LUA_TSTRING) {
        const char *sig_name = lua_tostring(L, 1);
        int res = sigdb_lookup_num_by_name(sig_name);
        if (res < 0) {
            (void) luaL_argerror(L, 1, "unknown signal name");
            LS_MUST_BE_UNREACHABLE();
        }
        *out = res;

    } else {
        (void) luaL_error(
            L, "expected number of string as argument, found %s",
            luaL_typename(L, 1));
        LS_MUST_BE_UNREACHABLE();
    }
}

static int l_kill(lua_State *L) /*__THROWABLE__*/
{
    Priv *p = lua_touserdata(L, lua_upvalueindex(1));

    if (p->file_path) {
        return luaL_error(L, "file mode is used");
    }

    int sig_num;
    fetch_sig_num(L, &sig_num);

    // If /is_ok/ == 1: killed successfully.
    // If /is_ok/ == 0: error, error number in /err_num/.
    // If /is_ok/ == -1: process has not been created yet.
    int is_ok;
    int err_num;

    LS_PTH_CHECK(pthread_mutex_lock(&p->child_state.mtx));
    pid_t pid = p->child_state.pid;
    if (pid > 0) {
        if (kill(pid, sig_num) < 0) {
            is_ok = 0;
        } else {
            is_ok = 1;
        }
        err_num = errno;
    } else {
        if (pid == PID_NOT_SPAWNED_YET) {
            is_ok = -1;
        } else {
            is_ok = 0;
        }
        err_num = ESRCH;
    }
    LS_PTH_CHECK(pthread_mutex_unlock(&p->child_state.mtx));

    if (is_ok < 0) {
        return luaL_error(L, "process has not been created yet");
    }

    if (is_ok) {
        lua_pushboolean(L, 1);
        return 1;
    } else {
        const char *err_descr = ls_tls_strerror(err_num);
        lua_pushboolean(L, 0);
        lua_pushstring(L, err_descr);
        return 2;
    }
}

static int l_get_sigrt_bounds(lua_State *L) /*__THROWABLE__*/
{
    lua_pushinteger(L, SIGRTMIN);
    lua_pushinteger(L, SIGRTMAX);
    return 2;
}

static void register_funcs(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv;

    // See 'DOCS/c_notes/lua_cfuncs.md' in the root of the repo for what __OK__ means.

    // L: table

    lua_pushlightuserdata(L, p); // L: table ud
    /*__OK__*/ lua_pushcclosure(L, l_write_to_stdin, 1); // L: table func
    lua_setfield(L, -2, "write_to_stdin"); // L: table

    lua_pushlightuserdata(L, p); // L: table ud
    /*__OK__*/ lua_pushcclosure(L, l_kill, 1); // L: table func
    lua_setfield(L, -2, "kill"); // L: table

    /*__OK__*/ lua_pushcfunction(L, l_get_sigrt_bounds); // L: table func
    lua_setfield(L, -2, "get_sigrt_bounds"); // L: table
}

static void report_reason_of_death(
    LuastatusPluginData *pd,
    int wait_rc,
    int wait_status,
    int wait_errno)
{
    if (wait_rc < 0) {
        LS_INFOF(pd, "waitpid: %s", ls_tls_strerror(wait_errno));
    } else {
        if (WIFEXITED(wait_status)) {
            int exit_code = WEXITSTATUS(wait_status);
            LS_INFOF(pd, "child process exited with code %d", exit_code);
        } else if (WIFSIGNALED(wait_status)) {
            int term_sig = WTERMSIG(wait_status);
            LS_INFOF(pd, "child process was killed with signal %d", term_sig);
        } else {
            LS_INFOF(pd, "child process terminated in an unexpected way (%d)", wait_status);
        }
    }
}

static int do_spawn(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;

    LS_ASSERT(!p->file_path);
    LS_ASSERT(p->argv.size > 0);

    NB_Writer_Pipe writer_pipe = NB_WRITER_PIPE_DUMMY_INIT;

    if (p->pipe_stdin) {
        if (nb_writer_make_pipe(&writer_pipe) < 0) {
            LS_FATALF(pd, "cannot create self-pipe: %s", ls_tls_strerror(errno));
            goto error;
        }
    }

    args_list_add(&p->argv, NULL);

    LaunchResult res;
    LaunchError err;

    if (launch(p->argv.data, p->pipe_stdin, &res, &err) < 0) {
        LS_FATALF(pd, "cannot spawn process: %s: %s", err.where, ls_tls_strerror(err.errnum));
        goto error;
    }

    ls_make_nonblock(res.fd_stdout);

    NB_Writer *writer;
    if (res.fd_stdin >= 0) {
        ls_make_nonblock(res.fd_stdin);
        writer = nb_writer_new(res.fd_stdin, writer_pipe);
    } else {
        writer = NULL;
    }

    LS_PTH_CHECK(pthread_mutex_lock(&p->child_state.mtx));
    p->child_state.stdin_fd = res.fd_stdin;
    p->child_state.stdin_writer = writer;
    p->child_state.pid = res.pid;
    LS_PTH_CHECK(pthread_mutex_unlock(&p->child_state.mtx));

    return res.fd_stdout;

error:
    nb_writer_pipe_destroy(writer_pipe);
    return -1;
}

static int do_open_file(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;

    LS_ASSERT(p->file_path);
    LS_ASSERT(p->argv.size == 0);

    int fd = ls_open_fifo(p->file_path);
    if (fd < 0) {
        LS_FATALF(pd, "cannot open file '%s': %s", p->file_path, ls_tls_strerror(errno));
        return -1;
    }
    return fd;
}

static int waitpid_restart_on_eintr(pid_t pid, int *out_status)
{
    LS_ASSERT(pid > 0);

    pid_t r;
    while ((r = waitpid(pid, out_status, 0)) < 0 && errno == EINTR) {
        // do nothing
    }
    return r < 0 ? -1 : 0;
}

static void do_wait(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;

    LS_ASSERT(!p->file_path);

    LS_PTH_CHECK(pthread_mutex_lock(&p->child_state.mtx));

    int wait_status = 0;
    int wait_rc = waitpid_restart_on_eintr(p->child_state.pid, &wait_status);
    int wait_errno = errno;

    if (p->child_state.stdin_writer) {
        nb_writer_destroy(p->child_state.stdin_writer);
        p->child_state.stdin_writer = NULL;
    }

    p->child_state.pid = PID_ALREADY_TERMINATED;

    LS_PTH_CHECK(pthread_mutex_unlock(&p->child_state.mtx));

    report_reason_of_death(pd, wait_rc, wait_status, wait_errno);
}

static void make_call_simple(
    LuastatusPluginData *pd,
    LuastatusPluginRunFuncs funcs,
    const char *what)
{
    lua_State *L = funcs.call_begin(pd->userdata);
    // L: ?
    lua_createtable(L, 0, 1); // L: ? table
    lua_pushstring(L, what); // L: ? table str
    lua_setfield(L, -2, "what"); // L: ? table

    funcs.call_end(pd->userdata);
}

static void make_call_line(
    LuastatusPluginData *pd,
    LuastatusPluginRunFuncs funcs,
    const char *line,
    size_t nline,
    bool non_whole)
{
    lua_State *L = funcs.call_begin(pd->userdata);
    // L: ?
    lua_createtable(L, 0, 2); // L: ? table

    lua_pushstring(L, "line"); // L: ? table str
    lua_setfield(L, -2, "what"); // L: ? table

    lua_pushlstring(L, line, nline); // L: ? table str
    lua_setfield(L, -2, "line"); // L: ? table

    if (non_whole) {
        lua_pushboolean(L, 1); // L: ? table true
        lua_setfield(L, -2, "non_whole"); // L: ? table
    }

    funcs.call_end(pd->userdata);
}

static void locked_get_writer_pfds(Priv *p, struct pollfd *dst)
{
    LS_PTH_CHECK(pthread_mutex_lock(&p->child_state.mtx));

    int fd_pollout;
    int fd_pollin;
    nb_writer_get_fds_for_poll(p->child_state.stdin_writer, &fd_pollout, &fd_pollin);
    dst[0] = (struct pollfd) {.fd = fd_pollout, .events = POLLOUT};
    dst[1] = (struct pollfd) {.fd = fd_pollin,  .events = POLLIN};

    LS_PTH_CHECK(pthread_mutex_unlock(&p->child_state.mtx));
}

static int locked_write(Priv *p, bool woken_up_on_selfpipe)
{
    LS_PTH_CHECK(pthread_mutex_lock(&p->child_state.mtx));

    int res = nb_writer_write(p->child_state.stdin_writer, woken_up_on_selfpipe);
    int saved_errno = errno;

    LS_PTH_CHECK(pthread_mutex_unlock(&p->child_state.mtx));

    errno = saved_errno;
    return res;
}

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    int fd;
    if (p->file_path) {
        fd = do_open_file(pd);
    } else {
        fd = do_spawn(pd);
    }
    if (fd < 0) {
        return;
    }

    if (p->greet) {
        make_call_simple(pd, funcs, "hello");
    }

    NB_Reader *reader = nb_reader_new(fd, p->delimiter);

    bool we_have_writer = !p->file_path && p->pipe_stdin;

    for (;;) {
        struct pollfd pfds[3] = {
            {.fd = fd, .events = POLLIN},
            {.fd = -1},
            {.fd = -1},
        };
        if (we_have_writer) {
            locked_get_writer_pfds(p, pfds + 1);
        }

        int poll_rc = ls_poll(pfds, LS_ARRAY_SIZE(pfds), LS_TD_FOREVER);
        if (poll_rc < 0) {
            LS_FATALF(pd, "ls_poll: %s", ls_tls_strerror(errno));
            goto maybe_wait;
        }

        if (pfds[0].revents) {
            NB_Reader_Line line;
            int read_rc = nb_reader_read(reader, &line);
            if (read_rc < 0) {
                int saved_errno = errno;
                if (p->report_non_whole && line.len) {
                    make_call_line(pd, funcs, line.ptr, line.len, false);
                }
                if (saved_errno) {
                    LS_FATALF(pd, "read error: %s", ls_tls_strerror(saved_errno));
                } else {
                    LS_FATALF(pd, "child process closed its stdout");
                }
                goto maybe_wait;
            } else if (read_rc > 0) {
                make_call_line(pd, funcs, line.ptr, line.len, true);
                nb_reader_consumed_line(reader);
            }
        }

        if (pfds[1].revents || pfds[2].revents) {
            if (locked_write(p, pfds[2].revents != 0) < 0) {
                LS_FATALF(pd, "write error: %s", ls_tls_strerror(errno));
                goto maybe_wait;
            }
        }
    }

maybe_wait:
    if (!p->file_path) {
        do_wait(pd);
    }

    if (p->bye) {
        make_call_simple(pd, funcs, "bye");

        // It's fine, it is the desired behavior to simply hang here. "bye" is typically used by a
        // widget to have a last chance to do something and/or show something to the user after the
        // process has died (or we got EOF from the file); not hanging and returning here would mean
        // anything shown to the user in the "bye" callback is lost (luastatus will tell the barlib
        // to show an error segment instead).
        for (;;) {
            pause();
        }
    }
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .run = run,
    .destroy = destroy,
};
