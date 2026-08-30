/*
 * Copyright (C) 2026  luastatus developers
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

#include "unixdom_open.h"

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "include/sayf_macros.h"

#include "libls/ls_panic.h"
#include "libls/ls_osdep.h"
#include "libls/ls_io_utils.h"
#include "libls/ls_tls_ebuf.h"

#include "external_context.h"

static int connect_restart_on_eintr(
    int sockfd,
    const struct sockaddr *addr,
    socklen_t addrlen)
{
    int res;
    while ((res = connect(sockfd, addr, addrlen)) < 0 && errno == EINTR) {
        // do nothing
    }
    return res;
}

int unixdom_open(ExternalContext ectx, const char *path)
{
    LS_ASSERT(path != NULL);

    int fd = -1;

    struct sockaddr_un saun = {.sun_family = AF_UNIX};
    size_t npath = strlen(path);
    if (npath + 1 > sizeof(saun.sun_path)) {
        LS_FATALF(ectx, "socket path is too long: %s", path);
        goto error;
    }
    memcpy(saun.sun_path, path, npath + 1);
    fd = ls_cloexec_socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        LS_FATALF(ectx, "socket: %s", ls_tls_strerror(errno));
        goto error;
    }
    if (connect_restart_on_eintr(fd, (struct sockaddr *) &saun, sizeof(saun)) < 0) {
        LS_FATALF(ectx, "connect: %s: %s", path, ls_tls_strerror(errno));
        goto error;
    }
    return fd;
error:
    ls_close(fd);
    return -1;
}
