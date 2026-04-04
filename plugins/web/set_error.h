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

#include <lua.h>
#include <curl/curl.h>
#include "libls/ls_xallocf.h"

#define set_error(out_errmsg, ...) (*(out_errmsg) = ls_xallocf(__VA_ARGS__))

void set_type_error(char **out_errmsg, lua_State *L, int pos, int expected_type, const char *prefix);

void set_curl_error(char **out_errmsg, CURLcode rc);
