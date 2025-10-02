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

#define _GNU_SOURCE

#include "cloexec_accept.h"
#include "probes.generated.h"

#include "libls/ls_io_utils.h"

#include <sys/socket.h>
#include <stddef.h>

int cloexec_accept(int sockfd)
{
#if HAVE_GNU_ACCEPT4
    return accept4(sockfd, NULL, NULL, SOCK_CLOEXEC);
#else
    int fd = accept(sockfd, NULL, NULL);
    if (fd >= 0) {
        if (ls_make_cloexec(fd) < 0) {
            close(fd);
            return -1;
        }
    }
    return fd;
#endif
}
