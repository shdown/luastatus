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

#include "strset.h"

#include <stdlib.h>
#include <stdbool.h>

#include "libls/ls_alloc_utils.h"
#include "libls/ls_panic.h"
#include "libls/ls_freemem.h"

#include "strset_mergesort.h"
#include "strset_binsearch.h"

// See file 'DEV_NOTES.txt' for why we roll our own sorting/binsearch
// implementation.

Strset strset_new(void)
{
    return (Strset) {0};
}

void strset_add(Strset *x, const char *s)
{
    LS_ASSERT(s != NULL);

    if (x->size == x->capacity) {
        x->data = LS_M_X2REALLOC(x->data, &x->capacity);
    }
    x->data[x->size++] = ls_xstrdup(s);
}

void strset_freeze(Strset *x)
{
    strset_mergesort(x->data, x->size);
}

bool strset_contains(Strset *x, const char *s)
{
    LS_ASSERT(s != NULL);

    return strset_binsearch(x->data, x->size, s);
}

void strset_reset(Strset *x)
{
    for (size_t i = 0; i < x->size; ++i) {
        free(x->data[i]);
    }
    x->data = LS_M_FREEMEM(x->data, &x->size, &x->capacity);
}

void strset_destroy(Strset x)
{
    for (size_t i = 0; i < x.size; ++i) {
        free(x.data[i]);
    }
    free(x.data);
}
