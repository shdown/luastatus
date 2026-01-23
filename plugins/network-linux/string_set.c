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

#include "string_set.h"

#include <string.h>
#include <stdlib.h>
#include "libls/ls_alloc_utils.h"
#include "libls/ls_compdep.h"
#include "libls/ls_freemem.h"
#include "libls/ls_assert.h"

StringSet string_set_new(void)
{
    return (StringSet) {NULL, 0, 0};
}

void string_set_reset(StringSet *s)
{
    for (size_t i = 0; i < s->size; ++i) {
        free(s->data[i]);
    }
    s->data = LS_M_FREEMEM(s->data, &s->size, &s->capacity);
}

void string_set_add(StringSet *s, const char *val)
{
    LS_ASSERT(val != NULL);

    if (s->size == s->capacity) {
        s->data = LS_M_X2REALLOC(s->data, &s->capacity);
    }
    s->data[s->size++] = ls_xstrdup(val);
}

static int elem_cmp(const void *a, const void *b)
{
    return strcmp(* (const char *const *) a,
                  * (const char *const *) b);
}

void string_set_freeze(StringSet *s)
{
    if (s->size) {
        qsort(s->data, s->size, sizeof(char *), elem_cmp);
    }
}

bool string_set_contains(StringSet s, const char *val)
{
    LS_ASSERT(val != NULL);

    if (!s.size) {
        return false;
    }
    return bsearch(&val, s.data, s.size, sizeof(char *), elem_cmp) != NULL;
}

void string_set_destroy(StringSet s)
{
    for (size_t i = 0; i < s.size; ++i) {
        free(s.data[i]);
    }
    free(s.data);
}
