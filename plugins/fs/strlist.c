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

#include "strlist.h"
#include <stdlib.h>
#include <string.h>
#include "libls/ls_alloc_utils.h"
#include "libls/ls_assert.h"

Strlist strlist_new(size_t max_size)
{
    return (Strlist) {
        .data = NULL,
        .size = 0,
        .capacity = 0,
        .max_size = max_size,
    };
}

#define BAD_INDEX ((size_t) -1)

static size_t find(Strlist *x, const char *s)
{
    char **data = x->data;
    size_t size = x->size;
    for (size_t i = 0; i < size; ++i) {
        if (strcmp(data[i], s) == 0) {
            return i;
        }
    }
    return BAD_INDEX;
}

int strlist_push(Strlist *x, const char *s)
{
    LS_ASSERT(s != NULL);

    if (x->size == x->max_size) {
        return -1;
    }

    if (find(x, s) != BAD_INDEX) {
        return 0;
    }

    if (x->size == x->capacity) {
        x->data = LS_M_X2REALLOC(x->data, &x->capacity);
    }
    x->data[x->size++] = ls_xstrdup(s);
    return 1;
}

static void remove_at(Strlist *x, size_t i)
{
    LS_ASSERT(i < x->size);

    // Free the string at index /i/.
    free(x->data[i]);

    // Move the last element to position /i/ and decrease the size.
    size_t i_last = x->size - 1;
    x->data[i] = x->data[i_last];
    --x->size;
}

int strlist_remove(Strlist *x, const char *s)
{
    LS_ASSERT(s != NULL);

    size_t i = find(x, s);
    if (i == BAD_INDEX) {
        return 0;
    }

    remove_at(x, i);
    return 1;
}

void strlist_destroy(Strlist x)
{
    for (size_t i = 0; i < x.size; ++i) {
        free(x.data[i]);
    }
    free(x.data);
}
