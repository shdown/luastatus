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

#ifndef string_set_h_
#define string_set_h_

#include <stdlib.h>
#include <stdbool.h>

#include "libls/alloc_utils.h"
#include "libls/compdep.h"

typedef struct {
    char **data;
    size_t size;
    size_t capacity;
} StringSet;

LS_INHEADER StringSet string_set_new()
{
    return (StringSet) {NULL, 0, 0};
}

// Clears and unfreezes the set.
LS_INHEADER void string_set_reset(StringSet *s)
{
    for (size_t i = 0; i < s->size; ++i)
        free(s->data[i]);
    s->size = 0;
}

LS_INHEADER void string_set_add(StringSet *s, const char *val)
{
    if (s->size == s->capacity) {
        s->data = ls_x2realloc(s->data, &s->capacity, sizeof(char *));
    }
    s->data[s->size++] = ls_xstrdup(val);
}

void string_set_freeze(StringSet *s);

bool string_set_contains(StringSet s, const char *val);

LS_INHEADER void string_set_destroy(StringSet s)
{
    for (size_t i = 0; i < s.size; ++i)
        free(s.data[i]);
    free(s.data);
}

#endif
