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

#include <stddef.h>
#include <string.h>

#include "string_.h"
#include "vector.h"
#include "compdep.h"

// An array of constant strings on a single buffer. Panics on allocation failure.

typedef struct {
    LSString buf;
    LS_VECTOR_OF(size_t) offsets;
} LSStringArray;

LS_INHEADER LSStringArray ls_strarr_new(void)
{
    return (LSStringArray) {
        .buf = LS_VECTOR_NEW(),
        .offsets = LS_VECTOR_NEW(),
    };
}

LS_INHEADER LSStringArray ls_strarr_new_reserve(size_t totlen, size_t nelems)
{
    return (LSStringArray) {
        .buf = LS_VECTOR_NEW_RESERVE(char, totlen),
        .offsets = LS_VECTOR_NEW_RESERVE(size_t, nelems),
    };
}

LS_INHEADER void ls_strarr_append(LSStringArray *sa, const char *buf, size_t nbuf)
{
    LS_VECTOR_PUSH(sa->offsets, sa->buf.size);
    ls_string_append_b(&sa->buf, buf, nbuf);
}

LS_INHEADER void ls_strarr_append_s(LSStringArray *sa, const char *s)
{
    ls_strarr_append(sa, s, strlen(s) + 1);
}

LS_INHEADER size_t ls_strarr_size(LSStringArray sa)
{
    return sa.offsets.size;
}

LS_INHEADER const char *ls_strarr_at(LSStringArray sa, size_t index, size_t *n)
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

LS_INHEADER void ls_strarr_clear(LSStringArray *sa)
{
    LS_VECTOR_CLEAR(sa->buf);
    LS_VECTOR_CLEAR(sa->offsets);
}

LS_INHEADER void ls_strarr_shrink(LSStringArray *sa)
{
    LS_VECTOR_SHRINK(sa->buf);
    LS_VECTOR_SHRINK(sa->offsets);
}

LS_INHEADER void ls_strarr_destroy(LSStringArray sa)
{
    LS_VECTOR_FREE(sa.buf);
    LS_VECTOR_FREE(sa.offsets);
}

#endif
