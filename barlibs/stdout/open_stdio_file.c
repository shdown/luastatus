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

#include "open_stdio_file.h"

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

#include "libls/ls_io_utils.h"
#include "libls/ls_tls_ebuf.h"

#include "include/barlib_data_v1.h"
#include "include/sayf_macros.h"

static bool open_input_fd(
        LuastatusBarlibData *bd,
        FILE **dst,
        int fd)
{
    if (ls_make_cloexec(fd) < 0) {
        LS_FATALF(bd, "can't make fd %d (in_fd) CLOEXEC: %s", fd, ls_tls_strerror(errno));
        return false;
    }

    *dst = fdopen(fd, "r");
    if (!*dst) {
        LS_FATALF(bd, "can't fdopen %d (in_fd): %s", fd, ls_tls_strerror(errno));
        return false;
    }

    return true;
}

static bool open_input_filename(
        LuastatusBarlibData *bd,
        FILE **dst,
        const char *filename)
{
    int fd = open(filename, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        LS_FATALF(bd, "can't open '%s' (in_filename): %s", filename, ls_tls_strerror(errno));
        return false;
    }

    *dst = fdopen(fd, "r");
    if (!*dst) {
        LS_FATALF(bd, "can't fdopen %d (opened in_filename): %s", fd, ls_tls_strerror(errno));
        close(fd);
        return false;
    }

    return true;
}

bool open_input(
        LuastatusBarlibData *bd,
        FILE **dst,
        int fd,
        const char *filename)
{
    if (fd >= 0 && filename) {
        LS_FATALF(bd, "both in_fd and in_filename were specified");
        return false;
    }
    if (fd >= 0) {
        return open_input_fd(bd, dst, fd);
    }
    if (filename) {
        return open_input_filename(bd, dst, filename);
    }
    return true;
}

bool open_output(
        LuastatusBarlibData *bd,
        FILE **dst,
        int fd)
{
    if (ls_make_cloexec(fd) < 0) {
        LS_FATALF(bd, "can't make fd %d (out_fd) CLOEXEC: %s", fd, ls_tls_strerror(errno));
        return false;
    }

    *dst = fdopen(fd, "w");
    if (!*dst) {
        LS_FATALF(bd, "can't fdopen %d (out_fd): %s", fd, ls_tls_strerror(errno));
        return false;
    }

    return true;
}
