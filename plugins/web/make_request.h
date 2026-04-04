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

#include <stdbool.h>
#include <curl/curl.h>
#include "libls/ls_string.h"
#include "libls/ls_strarr.h"
#include "include/plugin_data_v1.h"

enum {
    REQ_FLAG_NEEDS_HEADERS = 1 << 0,
    REQ_FLAG_DEBUG         = 1 << 1,
};

typedef struct {
    long status;
    LS_StringArray headers;
    LS_String body;
} Response;

bool make_request(
        LuastatusPluginData *pd,
        int req_flags,
        CURL *C,
        Response *out,
        char **out_errmsg);
