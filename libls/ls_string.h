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

#ifndef ls_string_h_
#define ls_string_h_

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include "ls_compdep.h"
#include "ls_alloc_utils.h"
#include "ls_algo.h"
#include "ls_freemem.h"
#include "ls_assert.h"

typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} LS_String;

LS_INHEADER LS_String ls_string_new(void)
{
    return (LS_String) {NULL, 0, 0};
}

LS_INHEADER LS_String ls_string_new_reserve(size_t n)
{
    char *buf = LS_XNEW(char, n);
    return (LS_String) {buf, 0, n};
}

LS_INHEADER void ls_string_reserve(LS_String *x, size_t n)
{
    if (x->capacity < n) {
        x->capacity = n;
        x->data = LS_M_XREALLOC(x->data, n);
    }
}

LS_INHEADER void ls_string_ensure_avail(LS_String *x, size_t n)
{
    while (x->capacity - x->size < n) {
        x->data = LS_M_X2REALLOC(x->data, &x->capacity);
    }
}

LS_INHEADER void ls_string_clear(LS_String *x)
{
    x->data = LS_M_FREEMEM(x->data, &x->size, &x->capacity);
}

LS_INHEADER void ls_string_assign_b(LS_String *x, const char *buf, size_t nbuf)
{
    ls_string_reserve(x, nbuf);
    // see DOCS/c_notes/empty-ranges-and-c-stdlib.md
    if (nbuf) {
        memcpy(x->data, buf, nbuf);
    }
    x->size = nbuf;
}

LS_INHEADER void ls_string_assign_s(LS_String *x, const char *s)
{
    LS_ASSERT(s != NULL);

    ls_string_assign_b(x, s, strlen(s));
}

LS_INHEADER void ls_string_append_b(LS_String *x, const char *buf, size_t nbuf)
{
    ls_string_ensure_avail(x, nbuf);
    // see DOCS/c_notes/empty-ranges-and-c-stdlib.md
    if (nbuf) {
        memcpy(x->data + x->size, buf, nbuf);
    }
    x->size += nbuf;
}

LS_INHEADER void ls_string_append_s(LS_String *x, const char *s)
{
    LS_ASSERT(s != NULL);

    ls_string_append_b(x, s, strlen(s));
}

LS_INHEADER void ls_string_append_c(LS_String *x, char c)
{
    if (x->size == x->capacity) {
        x->data = LS_M_X2REALLOC(x->data, &x->capacity);
    }
    x->data[x->size++] = c;
}

// Appends a formatted string to /x/.
void ls_string_append_vf(LS_String *x, const char *fmt, va_list vl);

// Appends a formatted string to /x/.
LS_INHEADER LS_ATTR_PRINTF(2, 3)
void ls_string_append_f(LS_String *x, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    ls_string_append_vf(x, fmt, vl);
    va_end(vl);
}

LS_INHEADER void ls_string_assign_vf(LS_String *x, const char *fmt, va_list vl)
{
    ls_string_clear(x);
    ls_string_append_vf(x, fmt, vl);
}

LS_INHEADER LS_ATTR_PRINTF(2, 3)
void ls_string_assign_f(LS_String *s, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    ls_string_assign_vf(s, fmt, vl);
    va_end(vl);
}

LS_INHEADER LS_String ls_string_new_from_s(const char *s)
{
    LS_String x = ls_string_new();
    ls_string_assign_s(&x, s);
    return x;
}

LS_INHEADER LS_String ls_string_new_from_b(const char *buf, size_t nbuf)
{
    LS_String x = ls_string_new();
    ls_string_assign_b(&x, buf, nbuf);
    return x;
}

LS_INHEADER LS_String ls_string_new_from_vf(const char *fmt, va_list vl)
{
    LS_String x = ls_string_new();
    ls_string_append_vf(&x, fmt, vl);
    return x;
}

LS_INHEADER LS_ATTR_PRINTF(1, 2)
LS_String ls_string_new_from_f(const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    LS_String x = ls_string_new_from_vf(fmt, vl);
    va_end(vl);
    return x;
}

LS_INHEADER bool ls_string_eq_b(LS_String x, const char *buf, size_t nbuf)
{
    // We have to check that the size is not zero before calling /memcmp/:
    // see DOCS/c_notes/empty-ranges-and-c-stdlib.md
    return x.size == nbuf && (nbuf == 0 || memcmp(x.data, buf, nbuf) == 0);
}

LS_INHEADER bool ls_string_eq(LS_String x, LS_String y)
{
    return ls_string_eq_b(x, y.data, y.size);
}

// Swaps two string efficiently (in O(1) time).
LS_INHEADER void ls_string_swap(LS_String *x, LS_String *y)
{
    LS_SWAP(LS_String, *x, *y);
}

LS_INHEADER void ls_string_free(LS_String x)
{
    free(x.data);
}

#endif
