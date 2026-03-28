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

#pragma once

#include <stddef.h>
#include "libls/ls_compdep.h"
#include "libls/ls_panic.h"

typedef struct {
    char *name;
    char *code;
} Wspec;

typedef struct {
    Wspec *data;
    size_t size;
    size_t capacity;
} WspecList;

WspecList wspec_list_new(void);

LS_INHEADER int wspec_list_size(WspecList *x)
{
    return x->size;
}

LS_INHEADER const char *wspec_list_get_name(WspecList *x, size_t idx)
{
    LS_ASSERT(idx < x->size);
    return x->data[idx].name;
}

LS_INHEADER const char *wspec_list_get_code(WspecList *x, size_t idx)
{
    LS_ASSERT(idx < x->size);
    return x->data[idx].code;
}

void wspec_list_add(WspecList *x, const char *name, const char *code);

const char *wspec_list_find_duplicates(WspecList *x);

int wspec_list_find(WspecList *x, const char *name);

void wspec_list_get_rid_of_code(WspecList *x, size_t idx);

void wspec_list_destroy(WspecList *x);
