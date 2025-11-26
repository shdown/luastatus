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

#include "ls_alloc_utils.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "ls_assert.h"

static inline bool mul_zu(size_t *dst, size_t a, size_t b)
{
    if (b && a > SIZE_MAX / b) {
        return false;
    }
    *dst = a * b;
    return true;
}

void *ls_xmalloc(size_t nelems, size_t elemsz)
{
    size_t total_bytes;
    if (!mul_zu(&total_bytes, nelems, elemsz)) {
        goto oom;
    }
    void *r = malloc(total_bytes);
    if (!r && total_bytes) {
        goto oom;
    }
    return r;

oom:
    ls_oom();
}

void *ls_xcalloc(size_t nelems, size_t elemsz)
{
    void *r = calloc(nelems, elemsz);
    if (nelems && elemsz && !r) {
        ls_oom();
    }
    return r;
}

void *ls_xrealloc(void *p, size_t nelems, size_t elemsz)
{
    size_t total_bytes;
    if (!mul_zu(&total_bytes, nelems, elemsz)) {
        goto oom;
    }
    if (!total_bytes) {
        free(p);
        return NULL;
    }
    void *r = realloc(p, total_bytes);
    if (!r) {
        goto oom;
    }
    return r;

oom:
    ls_oom();
}

void *ls_x2realloc(void *p, size_t *pnelems, size_t elemsz)
{
    LS_ASSERT(elemsz != 0);

    if (*pnelems) {
        if (!mul_zu(pnelems, *pnelems, 2)) {
            ls_oom();
        }
    } else {
        *pnelems = 1;
    }

    return ls_xrealloc(p, *pnelems, elemsz);
}

void *ls_xmemdup(const void *p, size_t n)
{
    void *r = malloc(n);
    if (n) {
        if (!r) {
            ls_oom();
        }
        memcpy(r, p, n);
    }
    return r;
}
