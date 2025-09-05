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

#ifndef ls_io_utils_h_
#define ls_io_utils_h_

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include "compdep.h"
#include "time_utils.h"

#if EAGAIN == EWOULDBLOCK
#   define LS_IS_EAGAIN(E_) ((E_) == EAGAIN)
#else
#   define LS_IS_EAGAIN(E_) ((E_) == EAGAIN || (E_) == EWOULDBLOCK)
#endif

// Makes a file descriptor close-on-exec.
// On success, /fd/ is returned.
// On failure, /-1/ is returned and /errno/ is set.
LS_INHEADER int ls_make_cloexec(int fd)
{
    int flags = fcntl(fd, F_GETFD);
    if (flags < 0) {
        return -1;
    }
    flags |= FD_CLOEXEC;
    if (fcntl(fd, F_SETFD, flags) < 0) {
        return -1;
    }
    return fd;
}

// Makes a file descriptor non-blocking.
// On success, /fd/ is returned.
// On failure, /-1/ is returned and /errno/ is set.
LS_INHEADER int ls_make_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags < 0) {
        return -1;
    }
    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) < 0) {
        return -1;
    }
    return fd;
}

// Close if non-negative: valgrind doesn't like /close()/ on negative FDs.
LS_INHEADER int ls_close(int fd)
{
    if (fd < 0) {
        return 0;
    }
    return close(fd);
}

// Poll restarting on /EINTR/ errors.
int ls_poll(struct pollfd *fds, nfds_t nfds, LS_TimeDelta tmo);

// Poll a single fd for /POLLIN/ events.
LS_INHEADER int ls_wait_input_on_fd(int fd, LS_TimeDelta tmo)
{
    struct pollfd pfd = {.fd = fd, .events = POLLIN};
    return ls_poll(&pfd, 1, tmo);
}

// Open a FIFO for reading, and check that this is really a FIFO.
int ls_open_fifo(const char *fifo);

#endif
