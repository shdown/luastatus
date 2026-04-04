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
#include <stdbool.h>
#include <lua.h>
#include <curl/curl.h>
#include "next_request_params.h"

typedef struct {
    const char *spelling;

    bool (*apply)(
        NextRequestParams *dst,
        lua_State *L,
        CURLoption which,
        char **out_errmsg);

    CURLoption which;
} Opt;

extern const Opt OPTS[];
extern const size_t OPTS_NUM;

enum { REQUIRED_OPTION_INDEX = 0 };
