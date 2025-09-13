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

#ifndef ls_strarr_h_
#define ls_strarr_h_

#include <stdlib.h>
#include <string.h>

#include "string_.h"
#include "alloc_utils.h"
#include "compdep.h"
#include "freemem.h"

// An array of constant strings on a single buffer. Panics on allocation failure.

typedef struct {
    LS_String buf;
    struct {
        size_t *data;
        size_t size;
        size_t capacity;
    } offsets;
} LS_StringArray;

LS_INHEADER LS_StringArray ls_strarr_new(void)
{
    return (LS_StringArray) {
        .buf = ls_string_new(),
        .offsets = {NULL, 0, 0},
    };
}

LS_INHEADER LS_StringArray ls_strarr_new_reserve(size_t totlen, size_t nelems)
{
    size_t *offsets_buf = LS_XNEW(size_t, nelems);
    return (LS_StringArray) {
        .buf = ls_string_new_reserve(totlen),
        .offsets = {offsets_buf, 0, nelems},
    };
}

LS_INHEADER void ls_strarr_append(LS_StringArray *sa, const char *buf, size_t nbuf)
{
    if (sa->offsets.size == sa->offsets.capacity) {
        sa->offsets.data = ls_x2realloc(sa->offsets.data, &sa->offsets.capacity, sizeof(size_t));
    }
    sa->offsets.data[sa->offsets.size++] = sa->buf.size;

    ls_string_append_b(&sa->buf, buf, nbuf);
}

LS_INHEADER void ls_strarr_append_s(LS_StringArray *sa, const char *s)
{
    ls_strarr_append(sa, s, strlen(s) + 1);
}

LS_INHEADER size_t ls_strarr_size(LS_StringArray sa)
{
    return sa.offsets.size;
}

LS_INHEADER const char *ls_strarr_at(LS_StringArray sa, size_t index, size_t *n)
{
    size_t begin = sa.offsets.data[index];
    size_t end = index + 1 == sa.offsets.size
                   ? sa.buf.size
                   : sa.offsets.data[index + 1];
    if (n) {
        *n = end - begin;
    }
    return sa.buf.data + begin;
}

LS_INHEADER void ls_strarr_clear(LS_StringArray *sa)
{
    ls_string_clear(&sa->buf);
    LS_FREEMEM(sa->offsets.data, sa->offsets.size, sa->offsets.capacity);
}

LS_INHEADER void ls_strarr_destroy(LS_StringArray sa)
{
    ls_string_free(sa.buf);
    free(sa.offsets.data);
}

#endif
