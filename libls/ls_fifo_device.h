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

#ifndef ls_fifo_device_h_
#define ls_fifo_device_h_

#include <unistd.h>
#include "ls_compdep.h"
#include "ls_time_utils.h"
#include "ls_io_utils.h"

typedef struct {
    int fd;
} LS_FifoDevice;

LS_INHEADER LS_FifoDevice ls_fifo_device_new(void)
{
    return (LS_FifoDevice) {-1};
}

LS_INHEADER int ls_fifo_device_open(LS_FifoDevice *dev, const char *path)
{
    if (dev->fd >= 0) {
        return 0;
    }
    if (!path) {
        return 0;
    }
    dev->fd = ls_open_fifo(path);
    if (dev->fd < 0) {
        return -1;
    }
    return 0;
}

LS_INHEADER int ls_fifo_device_wait(LS_FifoDevice *dev, LS_TimeDelta tmo)
{
    int r = ls_wait_input_on_fd(dev->fd, tmo);
    if (r > 0) {
        close(dev->fd);
        dev->fd = -1;
    }
    return r;
}

LS_INHEADER void ls_fifo_device_close(LS_FifoDevice *dev)
{
    ls_close(dev->fd);
}

#endif
