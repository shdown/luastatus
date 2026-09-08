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

#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

static inline size_t mul_zu_or_oom(size_t n, size_t m)
{
    if (m && n > SIZE_MAX / m) {
        xpanic_oom();
    }
    return n * m;
}

void *xmalloc(size_t n, size_t m)
{
    return xrealloc(NULL, n, m);
}

void *xrealloc(void *p, size_t n, size_t m)
{
    size_t total = mul_zu_or_oom(n, m);
    if (!total) {
        free(p);
        return NULL;
    }
    void *r = realloc(p, total);
    if (!r) {
        xpanic_oom();
    }
    return r;
}

void *x2realloc(void *p, size_t *pcapacity, size_t elemsz)
{
    if (*pcapacity) {
        *pcapacity = mul_zu_or_oom(*pcapacity, 2);
    } else {
        *pcapacity = 1;
    }
    return xrealloc(p, *pcapacity, elemsz);
}

char *xstrdup(const char *s)
{
    xassert(s != NULL);

    size_t ns = strlen(s);
    char *r = xmalloc(ns + 1, 1);
    memcpy(r, s, ns + 1);
    return r;
}

void *xmemdup(const void *p, size_t n)
{
    void *r = xmalloc(n, 1);
    if (n) {
        memcpy(r, p, n);
    }
    return r;
}

char *xallocf(const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    char *r = xallocvf(fmt, vl);
    va_end(vl);

    return r;
}

char *xallocvf(const char *fmt, va_list vl)
{
    va_list vl2;
    va_copy(vl2, vl);

    int n = vsnprintf(NULL, 0, fmt, vl);
    if (n < 0) {
        goto fail;
    }
    size_t nbuf = ((size_t) n) + 1;
    char *r = xmalloc(nbuf, 1);
    if (vsnprintf(r, nbuf, fmt, vl2) < 0) {
        goto fail;
    }
    va_end(vl2);
    return r;

fail:
    xpanicf("xallocvf(): vsnprintf() failed.\n");
}

void xpanicf(const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);

    xpanicvf(fmt, vl);

    va_end(vl);
}

void xpanicvf(const char *fmt, va_list vl)
{
    vfprintf(stderr, fmt, vl);
    abort();
}
