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

#ifndef string_set_h_
#define string_set_h_

#include <stdlib.h>
#include <stdbool.h>

#include "libls/ls_alloc_utils.h"
#include "libls/ls_compdep.h"
#include "libls/ls_freemem.h"
#include "libls/ls_assert.h"

typedef struct {
    char **data;
    size_t size;
    size_t capacity;
} StringSet;

LS_INHEADER StringSet string_set_new(void)
{
    return (StringSet) {NULL, 0, 0};
}

// Clears and unfreezes the set.
LS_INHEADER void string_set_reset(StringSet *s)
{
    for (size_t i = 0; i < s->size; ++i) {
        free(s->data[i]);
    }
    s->data = LS_M_FREEMEM(s->data, &s->size, &s->capacity);
}

LS_INHEADER void string_set_add(StringSet *s, const char *val)
{
    LS_ASSERT(val != NULL);

    if (s->size == s->capacity) {
        s->data = LS_M_X2REALLOC(s->data, &s->capacity);
    }
    s->data[s->size++] = ls_xstrdup(val);
}

void string_set_freeze(StringSet *s);

bool string_set_contains(StringSet s, const char *val);

LS_INHEADER void string_set_destroy(StringSet s)
{
    for (size_t i = 0; i < s.size; ++i) {
        free(s.data[i]);
    }
    free(s.data);
}

#endif
