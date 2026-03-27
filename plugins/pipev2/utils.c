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

#include "utils.h"
#include <stddef.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include "libls/ls_panic.h"

int utils_full_write(int fd, const char *data, size_t ndata)
{
    size_t nwritten = 0;
    while (nwritten < ndata) {
        ssize_t w = write(fd, data + nwritten, ndata - nwritten);
        if (w < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        nwritten += w;
    }
    return 0;
}

int utils_waitpid(pid_t pid, int *out_status)
{
    LS_ASSERT(pid > 0);

    pid_t r;
    while ((r = waitpid(pid, out_status, 0)) < 0 && errno == EINTR) {
        // do nothing
    }
    return r < 0 ? -1 : 0;
}
