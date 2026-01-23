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

#include "dss_list.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "libls/ls_alloc_utils.h"
#include "libls/ls_assert.h"

typedef struct {
    char *name;
    char *lua_program;
} Elem;

struct DSS_List {
    Elem *data;
    size_t size;
    size_t capacity;
};

DSS_List *DSS_list_new(void)
{
    DSS_List *x = LS_XNEW(DSS_List, 1);
    *x = (DSS_List) {0};
    return x;
}

size_t DSS_list_size(DSS_List *x)
{
    return x->size;
}

void DSS_list_push(
    DSS_List *x,
    const char *name,
    const char *lua_program)
{
    LS_ASSERT(name != NULL);
    LS_ASSERT(lua_program != NULL);

    if (x->size == x->capacity) {
        x->data = LS_M_X2REALLOC(x->data, &x->capacity);
    }

    Elem e = {
        .name = ls_xstrdup(name),
        .lua_program = ls_xstrdup(lua_program),
    };
    x->data[x->size++] = e;
}

bool DSS_list_names_unique(DSS_List *x, const char **out_duplicate)
{
    for (size_t i = 1; i < x->size; ++i) {
        for (size_t j = 0; j < i; ++j) {
            const char *s_i = x->data[i].name;
            const char *s_j = x->data[j].name;
            if (strcmp(s_i, s_j) == 0) {
                *out_duplicate = s_i;
                return false;
            }
        }
    }
    return true;
}

const char *DSS_list_get_name(
    DSS_List *x,
    size_t idx)
{
    LS_ASSERT(idx < x->size);

    return x->data[idx].name;
}

const char *DSS_list_get_lua_program(
    DSS_List *x,
    size_t idx)
{
    LS_ASSERT(idx < x->size);

    const char *res = x->data[idx].lua_program;
    LS_ASSERT(res != NULL);
    return res;
}

void DSS_list_free_lua_program(
    DSS_List *x,
    size_t idx)
{
    LS_ASSERT(idx < x->size);

    Elem *e = &x->data[idx];
    LS_ASSERT(e->lua_program != NULL);

    free(e->lua_program);
    e->lua_program = NULL;
}

void DSS_list_destroy(DSS_List *x)
{
    for (size_t i = 0; i < x->size; ++i) {
        Elem *e = &x->data[i];
        free(e->name);
        free(e->lua_program);
    }
    free(x->data);
    free(x);
}
