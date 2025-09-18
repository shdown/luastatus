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
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "libls/ls_string.h"
#include "libls/ls_alloc_utils.h"
#include "libls/ls_cstring_utils.h"
#include "libls/ls_tls_ebuf.h"
#include "libls/ls_time_utils.h"
#include "libls/ls_fifo_device.h"
#include "libls/ls_strarr.h"
#include "libls/ls_io_utils.h"

#include "connect.h"
#include "proto.h"

typedef struct {
    char *hostname;
    uint64_t port;
    char *password;
    double tmo;
    double retry_tmo;
    char *retry_fifo;
    LS_String idle_str;

    bool enable_tcp_keepalive;
    char *bind_addr;
    int bind_addr_family;
} Priv;

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->hostname);
    free(p->password);
    free(p->retry_fifo);
    ls_string_free(p->idle_str);
    free(p->bind_addr);
    free(p);
}

static int parse_events_elem(MoonVisit *mv, void *ud, int kpos, int vpos)
{
    mv->where = "'events' element";
    (void) kpos;

    Priv *p = ud;

    if (moon_visit_checktype_at(mv, NULL, vpos, LUA_TSTRING) < 0)
        return -1;

    const char *s = lua_tostring(mv->L, vpos);
    ls_string_append_c(&p->idle_str, ' ');
    ls_string_append_s(&p->idle_str, s);
    return 1;
}

static int parse_ipver(const char *s)
{
    if (strcmp(s, "ipv4") == 0) {
        return FAMILY_IPV4;
    }
    if (strcmp(s, "ipv6") == 0) {
        return FAMILY_IPV6;
    }
    return -1;
}

static int parse_bind_params(Priv *p, MoonVisit *mv, int table_pos)
{
    int old_top = lua_gettop(mv->L);

    // mv->L: ?
    if (moon_visit_scrutinize_table(mv, table_pos, "bind", true) < 0) {
        goto fail;
    }
    // mv->L: ? bind
    if (lua_isnil(mv->L, -1)) {
        goto ok;
    }

    if (moon_visit_str(mv, -1, "addr", &p->bind_addr, NULL, false) < 0) {
        goto fail;
    }

    const char *ipver;
    if (moon_visit_scrutinize_str(mv, -1, "ipver", &ipver, NULL, false) < 0) {
        goto fail;
    }
    // mv->L: ? bind ipver

    int family = parse_ipver(ipver);
    if (family < 0) {
        moon_visit_err(mv, "bind.ipver is invalid");
        goto fail;
    }
    p->bind_addr_family = family;

ok:
    lua_settop(mv->L, old_top);
    return 0;

fail:
    lua_settop(mv->L, old_top);
    return -1;
}

static int init(LuastatusPluginData *pd, lua_State *L)
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
        .enable_tcp_keepalive = false,
        .bind_addr = NULL,
        .bind_addr_family = FAMILY_NONE,
    };
    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse hostname
    if (moon_visit_str(&mv, -1, "hostname", &p->hostname, NULL, true) < 0)
        goto mverror;

    // Parse port
    if (moon_visit_uint(&mv, -1, "port", &p->port, true) < 0)
        goto mverror;
    if (p->port > 65535) {
        LS_FATALF(pd, "port is not a valid port number");
        goto error;
    }

    // Parse password
    if (moon_visit_str(&mv, -1, "password", &p->password, NULL, true) < 0)
        goto mverror;
    if (p->password && (strchr(p->password, '\n'))) {
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

    // Parse enable_tcp_keepalive
    if (moon_visit_bool(&mv, -1, "enable_tcp_keepalive", &p->enable_tcp_keepalive, true) < 0) {
        goto mverror;
    }

    // Parse bind
    if (parse_bind_params(p, &mv, -1) < 0) {
        goto mverror;
    }

    // Parse events
    int has_events = moon_visit_table_f(&mv, -1, "events", parse_events_elem, p, true);
    if (has_events < 0) {
        goto mverror;
    }
    if (!has_events) {
        ls_string_append_s(&p->idle_str, " mixer player");
    }
    ls_string_append_b(&p->idle_str, "\n", 2); // append '\n' and '\0'

    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static inline size_t rstrip_nl_strlen(const char *s)
{
    size_t n = strlen(s);
    if (n && s[n - 1] == '\n')
        --n;
    return n;
}

// If /line/ is of form "key: value\n", appends /key/ and /value/ to /sa/.
static void kv_strarr_line_append(LS_StringArray *sa, const char *line)
{
    const char *colon_pos = strchr(line, ':');
    if (!colon_pos || colon_pos[1] != ' ')
        return;
    const char *value_pos = colon_pos + 2;
    ls_strarr_append(sa, line, colon_pos - line);
    ls_strarr_append(sa, value_pos, rstrip_nl_strlen(value_pos));
}

static void kv_strarr_table_push(LS_StringArray sa, lua_State *L)
{
    size_t n = ls_strarr_size(sa);
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

static void report_status(
        LuastatusPluginData *pd,
        LuastatusPluginRunFuncs funcs,
        const char *what)
{
    lua_State *L = funcs.call_begin(pd->userdata);
    lua_createtable(L, 0, 1); // L: table
    lua_pushstring(L, what); // L: table what
    lua_setfield(L, -2, "what"); // L: table
    funcs.call_end(pd->userdata);
}

typedef struct {
    FILE *f;
    char *buf;
    size_t nbuf;
} Context;

static void log_io_error(LuastatusPluginData *pd, Context *ctx)
{
    if (feof(ctx->f)) {
        LS_ERRF(pd, "connection closed");
    } else {
        LS_ERRF(pd, "I/O error: %s", ls_tls_strerror(errno));
    }
}

static void log_bad_response(
        LuastatusPluginData *pd,
        Context *ctx,
        const char *where)
{
    enum { LIMIT = 1024 };

    const char *s = ctx->buf;
    size_t n = rstrip_nl_strlen(s);
    if (n > LIMIT)
        n = LIMIT;

    LS_ERRF(pd, "server said (%s): %.*s", where, (int) n, s);
}

static int loop_until_ok(
        LuastatusPluginData *pd,
        Context *ctx,
        LS_StringArray *kv)
{
    for (;;) {
        if (getline(&ctx->buf, &ctx->nbuf, ctx->f) < 0) {
            log_io_error(pd, ctx);
            return -1;
        }
        switch (response_type(ctx->buf)) {
        case RESP_OK:
            return 0;
        case RESP_ACK:
            log_bad_response(pd, ctx, "in loop");
            return -1;
        default:
            if (kv) {
                kv_strarr_line_append(kv, ctx->buf);
            }
        }
    }
}

static void interact(
        LuastatusPluginData *pd,
        LuastatusPluginRunFuncs funcs,
        int fd)
{
    Priv *p = pd->priv;

    Context ctx = {.buf = NULL, .nbuf = 0};
    LS_StringArray kv_song   = ls_strarr_new();
    LS_StringArray kv_status = ls_strarr_new();
    int fd_to_close = fd;

    if (!(ctx.f = fdopen(fd, "r+"))) {
        LS_ERRF(pd, "can't fdopen connection fd: %s", ls_tls_strerror(errno));
        goto done;
    }
    fd_to_close = -1;

    // read and check the greeting
    if (getline(&ctx.buf, &ctx.nbuf, ctx.f) < 0) {
        log_io_error(pd, &ctx);
        goto done;
    }

    if (!ls_strfollow(ctx.buf, "OK MPD ")) {
        log_bad_response(pd, &ctx, "to greeting");
        goto done;
    }

    // send the password, if specified
    if (p->password) {
        fputs("password ", ctx.f);
        write_quoted(ctx.f, p->password);
        putc('\n', ctx.f);

        fflush(ctx.f);
        if (ferror(ctx.f)) {
            log_io_error(pd, &ctx);
            goto done;
        }

        if (getline(&ctx.buf, &ctx.nbuf, ctx.f) < 0) {
            log_io_error(pd, &ctx);
            goto done;
        }
        if (response_type(ctx.buf) != RESP_OK) {
            log_bad_response(pd, &ctx, "to password");
            goto done;
        }
    }

    LS_TimeDelta tmo = ls_double_to_TD(p->tmo, LS_TD_FOREVER);

    for (;;) {
        // write "currentsong\n"
        fputs("currentsong\n", ctx.f);
        fflush(ctx.f);
        if (ferror(ctx.f)) {
            log_io_error(pd, &ctx);
            goto done;
        }

        // until OK, append data to 'kv_song'
        if (loop_until_ok(pd, &ctx, &kv_song) < 0)
            goto done;

        // write "status\n"
        fputs("status\n", ctx.f);
        fflush(ctx.f);
        if (ferror(ctx.f)) {
            log_io_error(pd, &ctx);
            goto done;
        }

        // until OK, append data to 'kv_status'
        if (loop_until_ok(pd, &ctx, &kv_status) < 0)
            goto done;

        // make a call
        lua_State *L = funcs.call_begin(pd->userdata);
        lua_createtable(L, 0, 3); // L: table

        lua_pushstring(L, "update"); // L: table "update"
        lua_setfield(L, -2, "what"); // L: table

        kv_strarr_table_push(kv_song, L); // L: table table
        lua_setfield(L, -2, "song"); // L: table

        kv_strarr_table_push(kv_status, L); // L: table table
        lua_setfield(L, -2, "status"); // L: table

        funcs.call_end(pd->userdata);

        // clear the arrays
        ls_strarr_clear(&kv_status);
        ls_strarr_clear(&kv_song);

        // write the idle string ("idle <...list of events...>\n")
        fputs(p->idle_str.data, ctx.f);
        fflush(ctx.f);
        if (ferror(ctx.f)) {
            log_io_error(pd, &ctx);
            goto done;
        }

        // if we need to, report timeouts until we have data on fd
        if (!ls_TD_is_forever(tmo)) {
            for (;;) {
                int nfds = ls_wait_input_on_fd(fd, tmo);
                if (nfds < 0) {
                    LS_ERRF(pd, "ls_wait_input_on_fd: %s", ls_tls_strerror(errno));
                    goto done;
                } else if (nfds == 0) {
                    report_status(pd, funcs, "timeout");
                } else {
                    break;
                }
            }
        }

        // wait for an OK
        if (loop_until_ok(pd, &ctx, NULL) < 0)
            goto done;
    }

done:
    if (ctx.f) {
        fclose(ctx.f);
    }
    ls_close(fd_to_close);
    free(ctx.buf);
    ls_strarr_destroy(kv_song);
    ls_strarr_destroy(kv_status);
}

static void do_enable_tcp_keepalive(LuastatusPluginData *pd, int fd)
{
    int value = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &value, sizeof(value)) < 0) {
        LS_WARNF(pd, "setsockopt: SO_KEEPALIVE: %s", ls_tls_strerror(errno));
    }
}

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    LS_FifoDevice retry_fifo_dev = ls_fifo_device_new();

    char portstr[8];
    snprintf(portstr, sizeof(portstr), "%d", (int) p->port);

    while (1) {
        report_status(pd, funcs, "connecting");

        int fd;
        if (p->hostname && p->hostname[0] == '/') {
            fd = unixdom_open(pd, p->hostname);
            if (fd < 0) {
                goto retry;
            }
        } else {
            fd = inetdom_open(pd, p->hostname, portstr, p->bind_addr, p->bind_addr_family);
            if (fd < 0) {
                goto retry;
            }
            if (p->enable_tcp_keepalive) {
                do_enable_tcp_keepalive(pd, fd);
            }
        }

        interact(pd, funcs, fd);

retry:
        report_status(pd, funcs, "error");

        LS_TimeDelta retry_tmo;
        if (!ls_double_to_TD_checked(p->retry_tmo, &retry_tmo)) {
            LS_FATALF(pd, "an error occurred; not retrying as requested");
            goto error;
        }

        if (ls_fifo_device_open(&retry_fifo_dev, p->retry_fifo) < 0) {
            LS_WARNF(pd, "ls_fifo_device_open: %s: %s", p->retry_fifo, ls_tls_strerror(errno));
        }

        if (ls_fifo_device_wait(&retry_fifo_dev, retry_tmo) < 0) {
            LS_FATALF(pd, "ls_fifo_device_wait: %s: %s", p->retry_fifo, ls_tls_strerror(errno));
            goto error;
        }
    }

error:
    ls_fifo_device_close(&retry_fifo_dev);
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
