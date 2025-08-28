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

#include "poll_utils.h"

#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>

#include "time_utils.h"
#include "io_utils.h"
#include "panic.h"

int ls_poll(struct pollfd *fds, nfds_t nfds, double tmo)
{
    int tmo_ms = ls_tmo_to_ms(tmo);

    sigset_t allsigs;
    sigfillset(&allsigs);

    sigset_t origmask;
    LS_PTH_CHECK(pthread_sigmask(SIG_SETMASK, &allsigs, &origmask));

    int r = poll(fds, nfds, tmo_ms);

    int saved_errno = errno;
    LS_PTH_CHECK(pthread_sigmask(SIG_SETMASK, &origmask, NULL));
    errno = saved_errno;

    return r;
}

int ls_wait_input_on_fd(int fd, double tmo)
{
    struct pollfd x = {.fd = fd, .events = POLLIN};
    return ls_poll(&x, 1, tmo);
}

int ls_fifo_open(int *fd, const char *fifo)
{
    int saved_errno;

    if (*fd >= 0) {
        return 0;
    }

    if (!fifo) {
        return 0;
    }

    *fd = open(fifo, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (*fd < 0) {
        goto error;
    }

    struct stat sb;
    if (fstat(*fd, &sb) < 0) {
        goto error;
    }

    if (!S_ISFIFO(sb.st_mode)) {
        errno = -EINVAL;
        goto error;
    }

    return 0;

error:
    saved_errno = errno;
    ls_close(*fd);
    *fd = -1;
    errno = saved_errno;
    return -1;
}

int ls_fifo_wait(int *fd, double tmo)
{
    int r = ls_wait_input_on_fd(*fd, tmo);
    if (r > 0) {
        close(*fd);
        *fd = -1;
    }
    return r;
}
