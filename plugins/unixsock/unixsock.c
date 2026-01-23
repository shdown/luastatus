/*
 * Copyright (C) 2015-2025  luastatus developers
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
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "libls/ls_alloc_utils.h"
#include "libls/ls_evloop_lfuncs.h"
#include "libls/ls_tls_ebuf.h"
#include "libls/ls_time_utils.h"
#include "libls/ls_io_utils.h"
#include "libls/ls_osdep.h"

#include "server.h"

typedef struct {
    char *path;
    bool try_unlink;
    bool greet;
    uint64_t max_clients;
    double tmo;
    LS_TimeDelta tmo_as_TD;
    LS_PushedTimeout pushed_tmo;
} Priv;

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->path);
    ls_pushed_timeout_destroy(&p->pushed_tmo);
    free(p);
}

static int init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .path = NULL,
        .try_unlink = true,
        .greet = false,
        .max_clients = 5,
        .tmo = -1,
    };
    ls_pushed_timeout_init(&p->pushed_tmo);

    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse path
    if (moon_visit_str(&mv, -1, "path", &p->path, NULL, false) < 0)
        goto mverror;
    if (p->path[0] == '\0') {
        LS_FATALF(pd, "path is empty");
        goto error;
    }

    // Parse try_unlink
    if (moon_visit_bool(&mv, -1, "try_unlink", &p->try_unlink, true) < 0)
        goto mverror;

    // Parse greet
    if (moon_visit_bool(&mv, -1, "greet", &p->greet, true) < 0)
        goto mverror;

    // Parse max_concur_conns
    if (moon_visit_uint(&mv, -1, "max_concur_conns", &p->max_clients, true) < 0) {
        goto mverror;
    }
    if (!p->max_clients) {
        LS_FATALF(pd, "max_concur_conns is zero");
        goto error;
    }
    if (p->max_clients > SERVER_MAX_CLIENTS_LIMIT) {
        LS_FATALF(pd, "max_concur_conns is too big (limit is %d)", (int) SERVER_MAX_CLIENTS_LIMIT);
        goto error;
    }

    // Parse timeout
    if (moon_visit_num(&mv, -1, "timeout", &p->tmo, true) < 0) {
        goto mverror;
    }
    p->tmo_as_TD = ls_double_to_TD(p->tmo, LS_TD_FOREVER);

    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static void register_funcs(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv;
    // L: table
    ls_pushed_timeout_push_luafunc(&p->pushed_tmo, L); // L: table func
    lua_setfield(L, -2, "push_timeout"); // L: table
}

static inline LS_TimeStamp new_deadline(Priv *p)
{
    LS_TimeDelta tmo = ls_pushed_timeout_fetch(&p->pushed_tmo, p->tmo_as_TD);
    if (ls_TD_is_forever(tmo)) {
        return LS_TS_BAD;
    }
    return ls_TS_plus_TD(ls_now(), tmo);
}

static inline LS_TimeDelta get_time_until_TS(LS_TimeStamp TS)
{
    if (ls_TS_is_bad(TS)) {
        return LS_TD_FOREVER;
    }
    return ls_TS_minus_TS_nonneg(TS, ls_now());
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

static void report_line(
        LuastatusPluginData *pd,
        LuastatusPluginRunFuncs funcs,
        const char *line,
        size_t nline)
{
    lua_State *L = funcs.call_begin(pd->userdata);
    lua_createtable(L, 0, 2); // L: table
    lua_pushstring(L, "line"); // L: table str
    lua_setfield(L, -2, "what"); // L: table
    lua_pushlstring(L, line, nline); // L: table str
    lua_setfield(L, -2, "line"); // L: table
    funcs.call_end(pd->userdata);
}

static int mk_server(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    int fd = ls_cloexec_socket(AF_UNIX, SOCK_STREAM, 0);

    if (fd < 0) {
        LS_FATALF(pd, "ls_cloexec_socket: %s", ls_tls_strerror(errno));
        goto error;
    }
    struct sockaddr_un saun = {.sun_family = AF_UNIX};

    if (p->try_unlink) {
        if (unlink(p->path) < 0 && errno != ENOENT) {
            LS_WARNF(pd, "unlink: %s: %s", p->path, ls_tls_strerror(errno));
        }
    }

    size_t npath = strlen(p->path);
    if (npath + 1 > sizeof(saun.sun_path)) {
        LS_FATALF(pd, "socket path is too long");
        goto error;
    }
    memcpy(saun.sun_path, p->path, npath + 1);

    if (bind(fd, (struct sockaddr *) &saun, sizeof(saun)) < 0) {
        LS_FATALF(pd, "bind: %s", ls_tls_strerror(errno));
        goto error;
    }

    if (listen(fd, SOMAXCONN) < 0) {
        LS_FATALF(pd, "listen: %s", ls_tls_strerror(errno));
        goto error;
    }

    return fd;

error:
    ls_close(fd);
    return -1;
}

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    Server *S = NULL;
    int srv_fd = mk_server(pd);

    if (srv_fd < 0) {
        goto error;
    }
    S = server_new(srv_fd, p->max_clients);

    LS_TimeStamp deadline = new_deadline(p);

    if (p->greet) {
        report_status(pd, funcs, "hello");
        deadline = new_deadline(p);
    }

    for (;;) {
        LS_TimeDelta tmo = get_time_until_TS(deadline);

        struct pollfd *pfds;
        size_t pfds_num;
        bool can_accept;

        int poll_rc = server_poll(S, tmo, &pfds, &pfds_num, &can_accept);
        if (poll_rc < 0) {
            LS_FATALF(pd, "poll: %s", ls_tls_strerror(errno));
            goto error;
        }

        if (poll_rc == 0) {
            report_status(pd, funcs, "timeout");
            deadline = new_deadline(p);
        }

        for (size_t i = 0; i < pfds_num; ++i) {
            if (!pfds[i].revents) {
                continue;
            }
            int read_rc = server_read_from_client(S, i);
            if (read_rc < 0) {
                if (errno == 0) {
                    LS_DEBUGF(pd, "clent disconnected before sending a full line");
                } else {
                    LS_WARNF(pd, "read: %s", ls_tls_strerror(errno));
                }
                server_drop_client(S, i);
                continue;

            } else if (read_rc > 0) {
                size_t nline;
                const char *line = server_get_full_line(S, i, &nline);

                report_line(pd, funcs, line, nline);
                server_drop_client(S, i);

                deadline = new_deadline(p);
            }
        }

        if (can_accept) {
            int accept_rc = server_accept_new_client(S);
            if (accept_rc < 0) {
                LS_FATALF(pd, "accept: %s", ls_tls_strerror(errno));
                goto error;

            } else if (accept_rc > 0) {
                LS_DEBUGF(pd, "accepted a new client");
            }
        }

        server_compact(S);
    }

error:
    ls_close(srv_fd);
    if (S) {
        server_destroy(S);
    }
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .run = run,
    .destroy = destroy,
};
