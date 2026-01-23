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

#ifndef dss_list_h_
#define dss_list_h_

#include <stdbool.h>
#include <stddef.h>

struct DSS_List;
typedef struct DSS_List DSS_List;

DSS_List *DSS_list_new(void);

size_t DSS_list_size(DSS_List *x);

void DSS_list_push(
    DSS_List *x,
    const char *name,
    const char *lua_program);

bool DSS_list_names_unique(
    DSS_List *x,
    const char **out_duplicate);

const char *DSS_list_get_name(
    DSS_List *x,
    size_t idx);

const char *DSS_list_get_lua_program(
    DSS_List *x,
    size_t idx);

void DSS_list_free_lua_program(
    DSS_List *x,
    size_t idx);

void DSS_list_destroy(DSS_List *x);

#endif
