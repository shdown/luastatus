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

#include "wspec_list.h"
#include "libls/ls_alloc_utils.h"
#include "libls/ls_panic.h"
#include <stdlib.h>
#include <string.h>

WspecList wspec_list_new(void)
{
    return (WspecList) {0};
}

void wspec_list_add(WspecList *x, const char *name, const char *code)
{
    LS_ASSERT(name != NULL);
    LS_ASSERT(code != NULL);

    Wspec new_elem = (Wspec) {
        .name = ls_xstrdup(name),
        .code = ls_xstrdup(code),
    };
    if (x->size == x->capacity) {
        x->data = LS_M_X2REALLOC(x->data, &x->capacity);
    }
    x->data[x->size++] = new_elem;
}

const char *wspec_list_find_duplicates(WspecList *x)
{
    size_t n = x->size;
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < i; ++j) {
            const char *s1 = x->data[i].name;
            const char *s2 = x->data[j].name;
            if (strcmp(s1, s2) == 0) {
                return s1;
            }
        }
    }
    return NULL;
}

int wspec_list_find(WspecList *x, const char *name)
{
    LS_ASSERT(name != NULL);

    size_t n = x->size;
    for (size_t i = 0; i < n; ++i) {
        if (strcmp(x->data[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void wspec_list_get_rid_of_code(WspecList *x, size_t idx)
{
    LS_ASSERT(idx < x->size);

    Wspec *elem = &x->data[idx];
    free(elem->code);
    elem->code = NULL;
}

void wspec_list_destroy(WspecList *x)
{
    size_t n = x->size;
    for (size_t i = 0; i < n; ++i) {
        Wspec *elem = &x->data[i];
        free(elem->name);
        free(elem->code);
    }
    free(x->data);
}
