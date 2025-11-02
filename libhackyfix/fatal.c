/*
 * Copyright (C) 2025  luastatus developers
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

#include "fatal.h"
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

static void full_write(int fd, const char *buf, size_t nbuf)
{
    size_t nwritten = 0;
    while (nwritten < nbuf) {
        ssize_t w = write(fd, buf + nwritten, nbuf - nwritten);
        if (w < 0) {
            break;
        }
        nwritten += w;
    }
}

void fatal(const char *msg)
{
    full_write(2, msg, strlen(msg));
    abort();
}
