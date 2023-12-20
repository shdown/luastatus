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

#include "libls/alloc_utils.h"
#include "libls/evloop_lfuncs.h"
#include "libls/string_.h"
#include "libls/tls_ebuf.h"
#include "libls/poll_utils.h"
#include "libls/time_utils.h"
#include "libls/io_utils.h"
#include "libls/osdep.h"

#include "cloexec_accept.h"

typedef struct {
    char *path;
    bool try_unlink;
    bool greet;
    uint64_t max_clients;
    double tmo;
    LSPushedTimeout pushed_tmo;
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
    if (moon_visit_uint(&mv, -1, "max_concur_conns", &p->max_clients, true) < 0)
        goto mverror;
    if (!p->max_clients) {
        LS_FATALF(pd, "max_concur_conns is zero");
        goto error;
    }

    // Parse timeout
    if (moon_visit_num(&mv, -1, "timeout", &p->tmo, true) < 0)
        goto mverror;

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

typedef struct {
    int fd;
    LSString buf;
} Client;

static inline void client_drop(Client *c)
{
    close(c->fd);
    c->fd = -1;
}

static inline void client_destroy(Client *c)
{
    close(c->fd);
    ls_string_free(c->buf);
}

typedef struct {
    size_t nclients;
    Client *clients;
    struct pollfd *pfds;
} ServerState;

static inline ServerState server_state_new(void)
{
    return (ServerState) {
        .nclients = 0,
        .clients = NULL,
        .pfds = LS_XNEW(struct pollfd, 1),
    };
}

static inline void server_state_update_pfds(ServerState *s)
{
    for (size_t i = 0; i < s->nclients; ++i) {
        s->pfds[i + 1] = (struct pollfd) {
            .fd = s->clients[i].fd,
            .events = POLLIN,
        };
    }
}

static inline void server_state_set_nclients(ServerState *s, size_t n)
{
    if (s->nclients != n) {
        s->nclients = n;
        s->clients = ls_xrealloc(s->clients, n, sizeof(Client));
        s->pfds = ls_xrealloc(s->pfds, n + 1, sizeof(struct pollfd));
    }
}

static void server_state_compact(ServerState *s)
{
    // Remove in-place clients that have been "dropped".
    size_t off = 0;
    size_t nclients = s->nclients;
    for (size_t i = 0; i < nclients; ++i) {
        if (s->clients[i].fd < 0) {
            client_destroy(&s->clients[i]);
            ++off;
        } else {
            s->clients[i - off] = s->clients[i];
        }
    }

    server_state_set_nclients(s, nclients - off);
    server_state_update_pfds(s);
}

static inline void server_state_add_client(ServerState *s, Client c)
{
    server_state_set_nclients(s, s->nclients + 1);
    s->clients[s->nclients - 1] = c;
    server_state_update_pfds(s);
}

static inline void server_state_destroy(ServerState *s)
{
    for (size_t i = 0; i < s->nclients; ++i)
        client_destroy(&s->clients[i]);
    free(s->clients);
    free(s->pfds);
}

static inline double new_tmo_pt(Priv *p)
{
    double tmo = ls_pushed_timeout_fetch(&p->pushed_tmo, p->tmo);
    if (!(tmo >= 0))
        return -1;
    return ls_monotonic_now() + tmo;
}

static inline double get_tmo_until_pt(double tmo_pt)
{
    if (tmo_pt < 0)
        return -1;
    double delta = tmo_pt - ls_monotonic_now();
    return delta < 0 ? 0 : delta;
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
        const char *line_begin,
        const char *line_end)
{
    lua_State *L = funcs.call_begin(pd->userdata);
    lua_createtable(L, 0, 2); // L: table
    lua_pushstring(L, "line"); // L: table str
    lua_setfield(L, -2, "what"); // L: table
    lua_pushlstring(L, line_begin, line_end - line_begin); // L: table str
    lua_setfield(L, -2, "line"); // L: table
    funcs.call_end(pd->userdata);
}

static int mk_server(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    int sockfd = ls_cloexec_socket(AF_UNIX, SOCK_STREAM, 0);

    if (sockfd < 0) {
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

    if (bind(sockfd, (struct sockaddr *) &saun, sizeof(saun)) < 0) {
        LS_FATALF(pd, "bind: %s", ls_tls_strerror(errno));
        goto error;
    }

    if (listen(sockfd, SOMAXCONN) < 0) {
        LS_FATALF(pd, "listen: %s", ls_tls_strerror(errno));
        goto error;
    }

    return sockfd;

error:
    close(sockfd);
    return -1;
}

enum { NCHUNK = 1024 };

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;
    ServerState st = server_state_new();
    int sockfd = mk_server(pd);

    if (sockfd < 0)
        goto error;

    ls_make_nonblock(sockfd);
    double tmo_pt = new_tmo_pt(p);

    if (p->greet) {
        report_status(pd, funcs, "hello");
        tmo_pt = new_tmo_pt(p);
    }

    for (;;) {
        if (st.nclients < p->max_clients)
            st.pfds[0] = (struct pollfd) {.fd = sockfd, .events = POLLIN};
        else
            st.pfds[0] = (struct pollfd) {.fd = -1};

        double tmo = get_tmo_until_pt(tmo_pt);
        int nfds = ls_poll(st.pfds, st.nclients + 1, tmo);
        if (nfds < 0) {
            LS_FATALF(pd, "ls_poll: %s", ls_tls_strerror(errno));
            goto error;
        }
        if (nfds == 0) {
            report_status(pd, funcs, "timeout");
            tmo_pt = new_tmo_pt(p);
        }

        if (st.pfds[0].revents & POLLIN) {
            int fd = cloexec_accept(sockfd);
            if (fd < 0) {
                if (LS_IS_EAGAIN(errno) || errno == ECONNABORTED)
                    continue;

                LS_FATALF(pd, "accept: %s", ls_tls_strerror(errno));
                goto error;
            } else {
                ls_make_nonblock(fd);
                server_state_add_client(&st, (Client) {
                    .fd = fd,
                    .buf = ls_string_new_reserve(NCHUNK),
                });
                continue;
            }
        }

        for (size_t i = 0; i < st.nclients; ++i) {
            if (!(st.pfds[i + 1].revents & POLLIN))
                continue;

            Client *c = &st.clients[i];

            ls_string_ensure_avail(&c->buf, NCHUNK);
            ssize_t r = read(c->fd, c->buf.data + c->buf.size, NCHUNK);
            if (r < 0) {
                if (LS_IS_EAGAIN(errno))
                    continue;
                LS_WARNF(pd, "read: %s", ls_tls_strerror(errno));
                client_drop(c);
                continue;
            }
            if (r == 0) {
                client_drop(c);
                continue;
            }

            char *ptr = memchr(c->buf.data + c->buf.size, '\n', r);
            if (ptr) {
                report_line(pd, funcs, c->buf.data, ptr);
                tmo_pt = new_tmo_pt(p);

                client_drop(c);
                continue;
            }

            c->buf.size += r;
        }

        server_state_compact(&st);
    }

error:
    close(sockfd);
    server_state_destroy(&st);
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .run = run,
    .destroy = destroy,
};
