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

#include "connect.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include "include/plugin_data_v1.h"
#include "include/sayf_macros.h"

#include "libls/tls_ebuf.h"
#include "libls/osdep.h"
#include "libls/panic.h"

int unixdom_open(LuastatusPluginData *pd, const char *path)
{
    int fd = -1;

    struct sockaddr_un saun = {.sun_family = AF_UNIX};
    size_t npath = strlen(path);
    if (npath + 1 > sizeof(saun.sun_path)) {
        LS_ERRF(pd, "socket path is too long: %s", path);
        goto error;
    }
    memcpy(saun.sun_path, path, npath + 1);
    fd = ls_cloexec_socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        LS_ERRF(pd, "socket: %s", ls_tls_strerror(errno));
        goto error;
    }
    if (connect(fd, (struct sockaddr *) &saun, sizeof(saun)) < 0) {
        LS_ERRF(pd, "connect: %s: %s", path, ls_tls_strerror(errno));
        goto error;
    }
    return fd;
error:
    close(fd);
    return -1;
}

static int do_bind_to_addr(int fd, const char *addr_str, int family)
{
    if (family == FAMILY_NONE) {
        return 0;
    }
    assert(addr_str);

    if (family == FAMILY_IPV4) {
        struct sockaddr_in sa = {
            .sin_family = AF_INET,
        };
        if (!inet_pton(AF_INET, addr_str, &sa.sin_addr)) {
            goto bad_str;
        }
        return bind(fd, (struct sockaddr *) &sa, sizeof(sa));

    } else if (family == FAMILY_IPV6) {
        struct sockaddr_in6 sa = {
            .sin6_family = AF_INET6,
        };
        if (!inet_pton(AF_INET6, addr_str, &sa.sin6_addr)) {
            goto bad_str;
        }
        return bind(fd, (struct sockaddr *) &sa, sizeof(sa));

    } else {
        char msg[70];
        snprintf(msg, sizeof(msg), "mpd plugin: do_bind_to_addr() got invalid family=%d", family);
        LS_PANIC(msg);
    }

bad_str:
    errno = EINVAL;
    return -1;
}

int inetdom_open(
        LuastatusPluginData *pd,
        const char *hostname,
        const char *service,
        const char *bind_addr,
        int bind_addr_family)
{
    struct addrinfo *ai = NULL;
    int fd = -1;

    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = 0,
        .ai_flags = AI_ADDRCONFIG,
    };

    int gai_r = getaddrinfo(hostname, service, &hints, &ai);
    if (gai_r) {
        if (gai_r == EAI_SYSTEM) {
            LS_ERRF(pd, "getaddrinfo: %s", ls_tls_strerror(errno));
        } else {
            LS_ERRF(pd, "getaddrinfo: %s", gai_strerror(gai_r));
        }
        ai = NULL;
        goto cleanup;
    }

    for (struct addrinfo *pai = ai; pai; pai = pai->ai_next) {
        fd = ls_cloexec_socket(pai->ai_family, pai->ai_socktype, pai->ai_protocol);
        if (fd < 0) {
            LS_WARNF(pd, "(candiate) socket: %s", ls_tls_strerror(errno));
            continue;
        }
        if (do_bind_to_addr(fd, bind_addr, bind_addr_family) < 0) {
            LS_WARNF(pd, "(candiate) cannot bind to address: %s", ls_tls_strerror(errno));
            close(fd);
            fd = -1;
            continue;
        }
        if (connect(fd, pai->ai_addr, pai->ai_addrlen) < 0) {
            LS_WARNF(pd, "(candiate) connect: %s", ls_tls_strerror(errno));
            close(fd);
            fd = -1;
            continue;
        }
        break;
    }
    if (fd < 0) {
        LS_ERRF(pd, "can't connect to any of the candidates");
    }

cleanup:
    if (ai) {
        freeaddrinfo(ai);
    }
    return fd;
}
