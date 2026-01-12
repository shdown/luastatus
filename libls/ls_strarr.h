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

#ifndef ls_strarr_h_
#define ls_strarr_h_

#include <stdlib.h>
#include <string.h>

#include "ls_string.h"
#include "ls_alloc_utils.h"
#include "ls_compdep.h"
#include "ls_freemem.h"
#include "ls_assert.h"

// An array of constant strings on a single buffer. Panics on allocation failure.

typedef struct {
    size_t *data;
    size_t size;
    size_t capacity;
} LS_StringArray_Offsets;

typedef struct {
    LS_String buf;
    LS_StringArray_Offsets offsets;
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
    LS_StringArray_Offsets *o = &sa->offsets;
    if (o->size == o->capacity) {
        o->data = LS_M_X2REALLOC(o->data, &o->capacity);
    }
    o->data[o->size++] = sa->buf.size;

    ls_string_append_b(&sa->buf, buf, nbuf);
}

LS_INHEADER void ls_strarr_append_s(LS_StringArray *sa, const char *s)
{
    LS_ASSERT(s != NULL);

    ls_strarr_append(sa, s, strlen(s) + 1);
}

LS_INHEADER size_t ls_strarr_size(LS_StringArray sa)
{
    return sa.offsets.size;
}

LS_INHEADER const char *ls_strarr_at(LS_StringArray sa, size_t i, size_t *n)
{
    LS_StringArray_Offsets o = sa.offsets;

    LS_ASSERT(i < o.size);

    size_t begin = o.data[i];
    size_t end = (i + 1 == o.size) ? sa.buf.size : o.data[i + 1];
    if (n) {
        *n = end - begin;
    }
    return sa.buf.data + begin;
}

LS_INHEADER void ls_strarr_clear(LS_StringArray *sa)
{
    ls_string_clear(&sa->buf);

    LS_StringArray_Offsets *o = &sa->offsets;
    o->data = LS_M_FREEMEM(o->data, &o->size, &o->capacity);
}

LS_INHEADER void ls_strarr_destroy(LS_StringArray sa)
{
    ls_string_free(sa.buf);
    free(sa.offsets.data);
}

#endif
