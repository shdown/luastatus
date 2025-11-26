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

#include "ls_io_utils.h"
#include <stddef.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>
#include "ls_time_utils.h"
#include "ls_panic.h"
#include "ls_assert.h"

static int poll_forever(struct pollfd *fds, nfds_t nfds)
{
    int r;
    while ((r = poll(fds, nfds, -1)) < 0 && errno == EINTR) {
        // do nothing
    }
    return r;
}

int ls_poll(struct pollfd *fds, nfds_t nfds, LS_TimeDelta tmo)
{
    int tmo_ms = ls_TD_to_poll_ms_tmo(tmo);
    if (tmo_ms < 0) {
        return poll_forever(fds, nfds);
    }

    sigset_t allsigs;
    LS_ASSERT(sigfillset(&allsigs) == 0);

    sigset_t origmask;
    LS_PTH_CHECK(pthread_sigmask(SIG_SETMASK, &allsigs, &origmask));

    int r = poll(fds, nfds, tmo_ms);

    int saved_errno = errno;
    LS_PTH_CHECK(pthread_sigmask(SIG_SETMASK, &origmask, NULL));
    errno = saved_errno;

    return r;
}

int ls_open_fifo(const char *fifo)
{
    LS_ASSERT(fifo != NULL);

    int saved_errno;

    int fd = open(fifo, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0) {
        goto error;
    }

    struct stat sb;
    if (fstat(fd, &sb) < 0) {
        goto error;
    }

    if (!S_ISFIFO(sb.st_mode)) {
        errno = -EINVAL;
        goto error;
    }

    return fd;

error:
    saved_errno = errno;
    ls_close(fd);
    errno = saved_errno;
    return -1;
}
