/*
 * Copyright (C) 2025  luastatus developers
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
#include <stdlib.h>
#include <string.h>
#include "libls/ls_alloc_utils.h"
#include "libls/ls_assert.h"
#include "libls/ls_compdep.h"

typedef struct {
    char *name_;
    char *lua_program_;
} DataSourceSpec;

LS_INHEADER DataSourceSpec DSS_new(const char *name, const char *lua_program)
{
    LS_ASSERT(name != NULL);
    LS_ASSERT(lua_program != NULL);

    return (DataSourceSpec) {
        .name_ = ls_xstrdup(name),
        .lua_program_ = ls_xstrdup(lua_program),
    };
}

LS_INHEADER void DSS_destroy(DataSourceSpec dss)
{
    free(dss.name_);
    free(dss.lua_program_);
}

typedef struct {
    DataSourceSpec *data_;
    size_t size_;
    size_t capacity_;
} DSS_List;

LS_INHEADER DSS_List DSS_list_new(void)
{
    return (DSS_List) {0};
}

LS_INHEADER size_t DSS_list_size(DSS_List *x)
{
    return x->size_;
}

LS_INHEADER void DSS_list_push(DSS_List *x, DataSourceSpec dss)
{
    if (x->size_ == x->capacity_) {
        x->data_ = LS_M_X2REALLOC(x->data_, &x->capacity_);
    }
    x->data_[x->size_++] = dss;
}

LS_INHEADER const char *DSS_list_get_name(DSS_List *x, size_t i)
{
    LS_ASSERT(i < x->size_);
    return x->data_[i].name_;
}

LS_INHEADER const char *DSS_list_get_lua_program(DSS_List *x, size_t i)
{
    LS_ASSERT(i < x->size_);

    const char *res = x->data_[i].lua_program_;
    LS_ASSERT(res != NULL);
    return res;
}

LS_INHEADER void DSS_list_free_lua_program(DSS_List *x, size_t i)
{
    LS_ASSERT(i < x->size_);

    DataSourceSpec *dss = &x->data_[i];
    free(dss->lua_program_);
    dss->lua_program_ = NULL;
}

LS_INHEADER void DSS_list_destroy(DSS_List *x)
{
    for (size_t i = 0; i < x->size_; ++i) {
        DSS_destroy(x->data_[i]);
    }
    free(x->data_);
}

LS_INHEADER bool DSS_list_names_unique(const DSS_List *x, const char **out_duplicate)
{
    for (size_t i = 1; i < x->size_; ++i) {
        for (size_t j = 0; j < i; ++j) {
            const char *s_i = x->data_[i].name_;
            const char *s_j = x->data_[j].name_;
            if (strcmp(s_i, s_j) == 0) {
                *out_duplicate = s_i;
                return false;
            }
        }
    }
    return true;
}

#endif
