/*
 * Copyright (C) 2015-2020  luastatus developers
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

#include <lua.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/select.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "libls/string_.h"
#include "libls/alloc_utils.h"
#include "libls/vector.h"
#include "libls/cstring_utils.h"
#include "libls/algo.h"
#include "libls/time_utils.h"
#include "libls/evloop_utils.h"
#include "libls/strarr.h"

#include "connect.h"
#include "proto.h"

typedef struct {
    char *hostname;
    double port;
    char *password;
    double tmo;
    double retry_tmo;
    char *retry_fifo;
    LSString idle_str;
} Priv;

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->hostname);
    free(p->password);
    free(p->retry_fifo);
    LS_VECTOR_FREE(p->idle_str);
    free(p);
}

static
int
parse_events_elem(MoonVisit *mv, void *ud, int kpos, int vpos)
{
    mv->where = "'events' element";
    (void) kpos;

    Priv *p = ud;

    if (moon_visit_checktype_at(mv, "", vpos, LUA_TSTRING) < 0)
        return -1;

    const char *s = lua_tostring(mv->L, vpos);
    ls_string_append_c(&p->idle_str, ' ');
    ls_string_append_s(&p->idle_str, s);
    return 1;
}

static
int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .hostname = NULL,
        .port = 6600,
        .password = NULL,
        .tmo = -1,
        .retry_tmo = 10,
        .retry_fifo = NULL,
        .idle_str = ls_string_new_from_s("idle"),
    };
    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse hostname
    if (moon_visit_str(&mv, -1, "hostname", &p->hostname, NULL, true) < 0)
        goto mverror;

    // Parse port
    if (moon_visit_num(&mv, -1, "port", &p->port, true) < 0)
        goto mverror;
    if (!ls_is_between_d(p->port, 0, 65535)) {
        LS_FATALF(pd, "port (%g) is not a valid port number", p->port);
        goto error;
    }

    // Parse password
    if (moon_visit_str(&mv, -1, "password", &p->password, NULL, true) < 0)
        goto mverror;
    if ((strchr(p->password, '\n'))) {
        LS_FATALF(pd, "password contains a line break");
        goto error;
    }

    // Parse timeout
    if (moon_visit_num(&mv, -1, "timeout", &p->tmo, true) < 0)
        goto mverror;

    // Parse retry_in
    if (moon_visit_num(&mv, -1, "retry_in", &p->retry_tmo, true) < 0)
        goto mverror;

    // Parse retry_fifo
    if (moon_visit_str(&mv, -1, "retry_fifo", &p->retry_fifo, NULL, true) < 0)
        goto mverror;

    // Parse events
    int has_events =
        moon_visit_table_f(&mv, -1, "events", parse_events_elem, p, true);
    if (has_events < 0)
        goto mverror;
    if (!has_events)
        ls_string_append_s(&p->idle_str, " mixer player");
    ls_string_append_b(&p->idle_str, "\n", 2); // append '\n' and '\0'

    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
error:
    destroy(pd);
    return LUASTATUS_ERR;
}

// Returns the length of /s/ without trailing newlines.
static inline
size_t
rstrip_nl_strlen(const char *s)
{
    size_t len = strlen(s);
    while (len && s[len - 1] == '\n') {
        --len;
    }
    return len;
}

static inline
int
rstrip_nl_strlen_limit(const char *s)
{
    enum { LIMIT = 8192 };
    const size_t r = rstrip_nl_strlen(s);
    return r > LIMIT ? LIMIT : r;
}

// If /line/ is of form "key: value\n", appends /key/ and /value/ to /sa/.
static
void
kv_strarr_line_append(LSStringArray *sa, const char *line)
{
    const char *colon_pos = strchr(line, ':');
    if (!colon_pos || colon_pos[1] != ' ') {
        return;
    }
    const char *value_pos = colon_pos + 2;
    ls_strarr_append(sa, line, colon_pos - line);
    ls_strarr_append(sa, value_pos, rstrip_nl_strlen(value_pos));
}

static
void
kv_strarr_table_push(LSStringArray sa, lua_State *L)
{
    const size_t n = ls_strarr_size(sa);
    assert(n % 2 == 0);
    lua_newtable(L); // L: table
    for (size_t i = 0; i < n; i += 2) {
        size_t nkey;
        const char *key = ls_strarr_at(sa, i, &nkey);
        lua_pushlstring(L, key, nkey); // L: table key
        size_t nvalue;
        const char *value = ls_strarr_at(sa, i + 1, &nvalue);
        lua_pushlstring(L, value, nvalue); // L: table key value
        lua_settable(L, -3); // L: table
    }
}

static inline
void
report_status(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs, const char *what)
{
    lua_State *L = funcs.call_begin(pd->userdata);
    lua_createtable(L, 0, 1); // L: table
    lua_pushstring(L, what); // L: table what
    lua_setfield(L, -2, "what"); // L: table
    funcs.call_end(pd->userdata);
}

static
void
interact(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs, int fd)
{
    Priv *p = pd->priv;

    FILE *f = fdopen(fd, "r+");
    if (!f) {
        LS_ERRF(pd, "can't fdopen connection file descriptor %d: %s",
                fd, ls_strerror_onstack(errno));
        close(fd);
        return;
    }

    char *buf = NULL;
    size_t nbuf = 1024;
    LSStringArray kv_song   = ls_strarr_new();
    LSStringArray kv_status = ls_strarr_new();

#define GETLINE() \
    do { \
        if (getline(&buf, &nbuf, f) < 0) { \
            goto io_error; \
        } \
    } while (0)

#define WRITE(What_) \
    do { \
        fputs(What_, f); \
        fflush(f); \
        if (ferror(f)) { \
            goto io_error; \
        } \
    } while (0)

#define UNTIL_OK(...) \
    do { \
        GETLINE(); \
        const ResponseType rt_ = response_type(buf); \
        if (rt_ == RESP_OK) { \
            break; \
        } else if (rt_ == RESP_ACK) { \
            LS_ERRF(pd, "server said: %.*s", rstrip_nl_strlen_limit(buf), buf); \
            goto error; \
        } else { \
            __VA_ARGS__ \
        } \
    } while (/*note infinite cycle here*/ 1)

    // read and check the greeting
    GETLINE();
    if (!ls_strfollow(buf, "OK MPD ")) {
        LS_ERRF(pd, "bad greeting: %.*s", rstrip_nl_strlen_limit(buf), buf);
        goto error;
    }

    // send the password, if specified
    if (p->password) {
        fputs("password ", f);
        write_quoted(f, p->password);
        putc_unlocked('\n', f);
        fflush(f);
        if (ferror(f)) {
            goto io_error;
        }
        GETLINE();
        if (response_type(buf) != RESP_OK) {
            LS_ERRF(pd, "(password) server said: %.*s", rstrip_nl_strlen_limit(buf), buf);
            goto error;
        }
    }

    while (1) {
        WRITE("currentsong\n");
        UNTIL_OK(
            kv_strarr_line_append(&kv_song, buf);
        );

        WRITE("status\n");
        UNTIL_OK(
            kv_strarr_line_append(&kv_status, buf);
        );

        lua_State *L = funcs.call_begin(pd->userdata);
        lua_createtable(L, 0, 3); // L: table

        lua_pushstring(L, "update"); // L: table "update"
        lua_setfield(L, -2, "what"); // L: table

        kv_strarr_table_push(kv_song, L); // L: table table
        ls_strarr_clear(&kv_song);
        lua_setfield(L, -2, "song"); // L: table

        kv_strarr_table_push(kv_status, L); // L: table table
        ls_strarr_clear(&kv_status);
        lua_setfield(L, -2, "status"); // L: table

        funcs.call_end(pd->userdata);

        WRITE(p->idle_str.data);

        if (p->tmo >= 0) {
            while (1) {
                int nfds = ls_wait_input_on_fd(fd, p->tmo);
                if (nfds < 0) {
                    LS_ERRF(pd, "ls_wait_input_on_fd: %s", ls_strerror_onstack(errno));
                    goto error;
                } else if (nfds == 0) {
                    report_status(pd, funcs, "timeout");
                } else {
                    break;
                }
            }
        }

        UNTIL_OK(
            // do nothing
        );
    }
#undef GETLINE
#undef WRITE
#undef UNTIL_OK

io_error:
    if (feof(f)) {
        LS_ERRF(pd, "connection closed");
    } else {
        LS_ERRF(pd, "I/O error: %s", ls_strerror_onstack(errno));
    }

error:
    fclose(f);
    free(buf);
    ls_strarr_destroy(kv_song);
    ls_strarr_destroy(kv_status);
}

static
void
run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    int retry_fifo_fd = -1;

    char portstr[8];
    snprintf(portstr, sizeof(portstr), "%d", (int) p->port);

    while (1) {
        report_status(pd, funcs, "connecting");

        int fd = (p->hostname && p->hostname[0] == '/')
            ? unixdom_open(pd, p->hostname)
            : inetdom_open(pd, p->hostname, portstr);

        if (fd >= 0)
            interact(pd, funcs, fd);

        if (p->retry_tmo < 0) {
            LS_FATALF(pd, "an error occurred; not retrying as requested");
            goto error;
        }

        report_status(pd, funcs, "error");

        if (ls_fifo_open(&retry_fifo_fd, p->retry_fifo) < 0) {
            LS_WARNF(pd, "ls_fifo_open: %s: %s", p->retry_fifo, LS_FIFO_STRERROR_ONSTACK(errno));
        }
        if (ls_fifo_wait(&retry_fifo_fd, p->retry_tmo) < 0) {
            LS_FATALF(pd, "ls_fifo_wait: %s: %s", p->retry_fifo, ls_strerror_onstack(errno));
            goto error;
        }
    }

error:
    close(retry_fifo_fd);
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
