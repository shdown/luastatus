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

#ifndef inotify_compat_h_
#define inotify_compat_h_

#include <stdbool.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "libls/ls_io_utils.h"
#include "libls/ls_osdep.h"
#include "libls/ls_compdep.h"

#include "probes.generated.h"

LS_INHEADER int compat_inotify_init(bool nonblock, bool cloexec)
{
#if HAVE_INOTIFY_INIT1
    return inotify_init1((nonblock ? IN_NONBLOCK : 0) |
                         (cloexec  ? IN_CLOEXEC  : 0));
#else
    int saved_errno;
    int fd = inotify_init();
    if (fd < 0) {
        goto error;
    }
    if (nonblock && ls_make_nonblock(fd) < 0) {
        goto error;
    }
    if (cloexec && ls_make_cloexec(fd) < 0) {
        goto error;
    }
    return fd;

error:
    saved_errno = errno;
    ls_close(fd);
    errno = saved_errno;
    return -1;
#endif
}

#endif
