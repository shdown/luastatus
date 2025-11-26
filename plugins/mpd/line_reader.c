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

#include "line_reader.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "libls/ls_alloc_utils.h"
#include "libsafe/safev.h"

LineReader line_reader_new(size_t prealloc)
{
    return (LineReader) {
        .buf = LS_XNEW(char, prealloc),
        .capacity = prealloc,
    };
}

int line_reader_read_line(LineReader *LR, FILE *f, SAFEV *out)
{
    ssize_t r = getline(&LR->buf, &LR->capacity, f);
    if (r < 0) {
        return -1;
    }
    SAFEV raw_v = SAFEV_new_UNSAFE(LR->buf, r);
    *out = SAFEV_rstrip_once(raw_v, '\n');
    return 0;
}

void line_reader_destroy(LineReader *LR)
{
    free(LR->buf);
}
