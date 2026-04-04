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
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static inline size_t mul_zu_or_oom(size_t a, size_t b)
{
    if (b && a > SIZE_MAX / b) {
        panic_oom();
    }
    return a * b;
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
        panic_oom();
    }
    return r;
}

void *x2realloc(void *p, size_t *pcapacity, size_t elem_sz)
{
    if (*pcapacity == 0) {
        *pcapacity = 1;
    } else {
        *pcapacity = mul_zu_or_oom(*pcapacity, 2);
    }
    return xrealloc(p, *pcapacity, elem_sz);
}

void *xmemdup(const void *p, size_t n)
{
    if (!n) {
        return NULL;
    }
    void *r = xmalloc(n, 1);
    memcpy(r, p, n);
    return r;
}

char *xstrdup(const char *s)
{
    return xmemdup(s, strlen(s) + 1);
}
