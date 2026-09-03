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

#include "nb_reader.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include "libls/ls_alloc_utils.h"
#include "libls/ls_string.h"
#include "libls/ls_io_utils.h"
#include "libls/ls_panic.h"

enum { BUF_SIZE = 4096 };

struct NB_Reader {
    int fd;
    char delim;

    LS_String line;

    size_t buf_pos;
    size_t buf_avail;
    char buf[BUF_SIZE];
};

NB_Reader *nb_reader_new(int fd, char delim)
{
    LS_ASSERT(fd >= 0);

    NB_Reader *x = LS_XNEW(NB_Reader, 1);
    *x = (NB_Reader) {
        .fd = fd,
        .delim = delim,
        .line = ls_string_new_reserve(1024),
        .buf_pos = 0,
        .buf_avail = 0,
        .buf = {0},
    };
    return x;
}

static int buf_fill(NB_Reader *x)
{
    ssize_t r = read(x->fd, x->buf, BUF_SIZE);
    if (r < 0) {
        if (LS_IS_EAGAIN(errno)) {
            return 0;
        }
        return -1;
    } else if (r == 0) {
        errno = 0;
        return -1;
    }

    x->buf_pos = 0;
    x->buf_avail = r;
    return 1;
}

static inline int buf_getc(NB_Reader *x, char *out)
{
    if (x->buf_pos == x->buf_avail) {
        int rc = buf_fill(x);
        if (rc <= 0) {
            return rc;
        }
    }
    *out = x->buf[x->buf_pos++];
    return 1;
}

int nb_reader_read(NB_Reader *x, NB_Reader_Line *out_line)
{
    LS_ASSERT(x != NULL);

    int rc;
    LS_String *line = &x->line;
    for (;;) {
        char c;
        rc = buf_getc(x, &c);
        if (rc <= 0) {
            break;
        }
        if (c == x->delim) {
            rc = 1;
            break;
        }
        ls_string_append_c(line, c);
    }

    // Take care not to clobber errno here.
    *out_line = (NB_Reader_Line) {
        .ptr = line->data,
        .len = line->size,
    };
    return rc;
}

void nb_reader_consumed_line(NB_Reader *x)
{
    LS_ASSERT(x != NULL);

    ls_string_clear(&x->line);
}

void nb_reader_destroy(NB_Reader *x)
{
    LS_ASSERT(x != NULL);

    ls_string_free(x->line);
    free(x);
}
