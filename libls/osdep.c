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

#define _GNU_SOURCE
#include "osdep.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#include "io_utils.h"
#include "probes.generated.h"

int
ls_cloexec_pipe(int pipefd[2])
{
#if LS_HAVE_GNU_PIPE2
    return pipe2(pipefd, O_CLOEXEC);
#else
    if (pipe(pipefd) < 0) {
        return -1;
    }
    for (int i = 0; i < 2; ++i) {
        if (ls_make_cloexec(pipefd[i]) < 0) {
            int saved_errno = errno;
            close(pipefd[0]);
            close(pipefd[1]);
            errno = saved_errno;
            return -1;
        }
    }
    return 0;
#endif
}

int
ls_cloexec_socket(int domain, int type, int protocol)
{
#if LS_HAVE_GNU_SOCK_CLOEXEC
    return socket(domain, type | SOCK_CLOEXEC, protocol);
#else
    int fd = socket(domain, type, protocol);
    if (fd < 0) {
        return -1;
    }
    if (ls_make_cloexec(fd) < 0) {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return -1;
    }
    return fd;
#endif
}
