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
#include <lua.h>
#include <lauxlib.h>

#include "libls/ls_alloc_utils.h"
#include "libls/ls_panic.h"
#include "libls/ls_io_utils.h"
#include "libls/ls_tls_ebuf.h"

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "launch.h"
#include "utils.h"
#include "sigdb.h"

enum {
    PID_NOT_SPAWNED_YET = 0,
    PID_ALREADY_TERMINATED = -1,
};

typedef struct {
    LaunchArg *data;
    size_t size;
    size_t capacity;
} ArgsList;

typedef struct {
    ArgsList argv;

    // The /getdelim()/ function requires the /int delim/ parameter to be "representable
    // as an unsigned char" (i.e. to be non-negative).
    //
    // Since the signedness of /char/ depends on the implementation, we store the
    // delimiter as an /unsigned char/. Then we can pass it to /getdelim()/ without cast.
    unsigned char delimiter;

    bool pipe_stdin;
    bool greet;
    bool bye;

    pthread_mutex_t child_mtx;
    int child_stdin_fd;
    pid_t child_pid;
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

    LS_PTH_CHECK(pthread_mutex_destroy(&p->child_mtx));

    ls_close(p->child_stdin_fd);

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
        .delimiter = '\n',
        .pipe_stdin = false,
        .greet = false,
        .bye = false,
        .child_stdin_fd = -1,
        .child_pid = PID_NOT_SPAWNED_YET,
    };
    LS_PTH_CHECK(pthread_mutex_init(&p->child_mtx, NULL));

    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse argv
    if (moon_visit_table_f(&mv, -1, "argv", visit_argv_elem, p, false) < 0) {
        goto mverror;
    }
    if (!p->argv.size) {
        snprintf(errbuf, sizeof(errbuf), "'argv' array is empty");
        goto error;
    }

    // Parse delimiter
    if (moon_visit_str_f(&mv, -1, "delimiter", visit_delimiter, p, true) < 0) {
        goto mverror;
    }

    // Parse pipe_stdin
    if (moon_visit_bool(&mv, -1, "pipe_stdin", &p->pipe_stdin, true) < 0) {
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

    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static int l_write_to_stdin(lua_State *L)
{
    Priv *p = lua_touserdata(L, lua_upvalueindex(1));

    if (!p->pipe_stdin) {
        return luaL_error(L, "'pipe_stdin' option was not enabled");
    }

    size_t ndata;
    const char *data = luaL_checklstring(L, 1, &ndata);

    LS_PTH_CHECK(pthread_mutex_lock(&p->child_mtx));
    int fd = p->child_stdin_fd;
    LS_PTH_CHECK(pthread_mutex_unlock(&p->child_mtx));

    if (fd < 0) {
        return luaL_error(L, "process have not been created yet");
    }

    if (utils_full_write(fd, data, ndata) >= 0) {
        lua_pushboolean(L, 1);
        return 1;
    } else {
        const char *err_descr = ls_tls_strerror(errno);
        lua_pushboolean(L, 0);
        lua_pushstring(L, err_descr);
        return 2;
    }
}

static int fetch_sig_num(lua_State *L)
{
    if (lua_isnoneornil(L, 1)) {
        return SIGTERM;
    }

    if (lua_isnumber(L, 1)) {
        int res = lua_tointeger(L, 1);
        if (res < 0) {
            return luaL_argerror(L, 1, "number is negative or out of range");
        }
        return res;

    } else if (lua_isstring(L, 1)) {
        const char *sig_name = lua_tostring(L, 1);
        int res = sigdb_lookup_num_by_name(sig_name);
        if (res < 0) {
            return luaL_argerror(L, 1, "unknown signal name");
        }
        return res;

    } else {
        return luaL_error(
            L, "expected number of string as argument, found %s",
            luaL_typename(L, 1));
    }
}

static int l_kill(lua_State *L)
{
    Priv *p = lua_touserdata(L, lua_upvalueindex(1));

    int sig_num = fetch_sig_num(L);

    // If /is_ok/ == 1: killed successfully.
    // If /is_ok/ == 0: error, error number in /err_num/.
    // If /is_ok/ == -1: process have not been created yet.
    int is_ok;
    int err_num;

    LS_PTH_CHECK(pthread_mutex_lock(&p->child_mtx));
    pid_t pid = p->child_pid;
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
    LS_PTH_CHECK(pthread_mutex_unlock(&p->child_mtx));

    if (is_ok < 0) {
        return luaL_error(L, "process have not been created yet");
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

static int l_get_sigrt_bounds(lua_State *L)
{
    lua_pushinteger(L, SIGRTMIN);
    lua_pushinteger(L, SIGRTMAX);
    return 2;
}

static void register_funcs(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv;

    // L: table

    lua_pushlightuserdata(L, p); // L: table ud
    lua_pushcclosure(L, l_write_to_stdin, 1); // L: table func
    lua_setfield(L, -2, "write_to_stdin"); // L: table

    lua_pushlightuserdata(L, p); // L: table ud
    lua_pushcclosure(L, l_kill, 1); // L: table func
    lua_setfield(L, -2, "kill"); // L: table

    lua_pushcfunction(L, l_get_sigrt_bounds); // L: table func
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

static void do_cleanup_leftover(LuastatusPluginData *pd, LaunchResult leftover)
{
    LS_INFOF(pd, "killing the spawned process with SIGKILL and waiting for it to terminate");

    (void) kill(leftover.pid, SIGKILL);

    int wait_status = 0;
    int wait_rc = utils_waitpid(leftover.pid, &wait_status);
    int wait_errno = errno;

    report_reason_of_death(pd, wait_rc, wait_status, wait_errno);

    ls_close(leftover.fd_stdin);
    ls_close(leftover.fd_stdout);
}

static bool do_spawn(LuastatusPluginData *pd, FILE **out_f)
{
    Priv *p = pd->priv;

    args_list_add(&p->argv, NULL);

    LaunchResult res;
    LaunchError err;

    if (launch(p->argv.data, p->pipe_stdin, &res, &err) < 0) {
        LS_FATALF(pd, "cannot spawn process: %s: %s", err.where, ls_tls_strerror(err.errnum));
        return false;
    }

    FILE *f = fdopen(res.fd_stdout, "r");
    if (!f) {
        LS_FATALF(pd, "fdopen: %s", ls_tls_strerror(errno));
        do_cleanup_leftover(pd, res);
        return false;
    }
    *out_f = f;

    LS_PTH_CHECK(pthread_mutex_lock(&p->child_mtx));
    p->child_stdin_fd = res.fd_stdin;
    p->child_pid = res.pid;
    LS_PTH_CHECK(pthread_mutex_unlock(&p->child_mtx));

    return true;
}

static void do_wait(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;

    LS_PTH_CHECK(pthread_mutex_lock(&p->child_mtx));

    int wait_status = 0;
    int wait_rc = utils_waitpid(p->child_pid, &wait_status);
    int wait_errno = errno;

    p->child_pid = PID_ALREADY_TERMINATED;

    LS_PTH_CHECK(pthread_mutex_unlock(&p->child_mtx));

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
    size_t nline)
{
    lua_State *L = funcs.call_begin(pd->userdata);
    // L: ?
    lua_createtable(L, 0, 2); // L: ? table

    lua_pushstring(L, "line"); // L: ? table str
    lua_setfield(L, -2, "what"); // L: ? table

    lua_pushlstring(L, line, nline); // L: ? table str
    lua_setfield(L, -2, "line"); // L: ? table

    funcs.call_end(pd->userdata);
}

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    FILE *f;
    if (!do_spawn(pd, &f)) {
        return;
    }

    if (p->greet) {
        make_call_simple(pd, funcs, "hello");
    }

    char *buf = NULL;
    size_t nbuf = 512;

    for (;;) {
        ssize_t r = getdelim(&buf, &nbuf, p->delimiter, f);
        if (r <= 0) {
            break;
        }
        make_call_line(pd, funcs, buf, r - 1);
    }

    if (feof(f)) {
        LS_ERRF(pd, "child process closed its stdout");
    } else {
        LS_ERRF(pd, "read error: %s", ls_tls_strerror(errno));
    }

    fclose(f);

    do_wait(pd);

    if (p->bye) {
        make_call_simple(pd, funcs, "bye");
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
