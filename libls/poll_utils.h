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

#ifndef ls_poll_utils_h_
#define ls_poll_utils_h_

#include <errno.h>
#include <poll.h>

#include "cstring_utils.h"

int ls_poll(struct pollfd *fds, nfds_t nfds, double tmo);

int ls_wait_input_on_fd(int fd, double tmo);

int ls_fifo_open(int *fd, const char *fifo);

#define LS_FIFO_STRERROR_ONSTACK(E_) \
    ((E_) == -EINVAL ? "Not a FIFO" : ls_strerror_onstack(E_))

int ls_fifo_wait(int *fd, double tmo);

#endif
