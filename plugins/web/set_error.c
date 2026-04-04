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

#include "set_error.h"

#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <curl/curl.h>
#include "libls/ls_panic.h"

void set_type_error(char **out_errmsg, lua_State *L, int pos, int expected_type, const char *prefix)
{
    LS_ASSERT(prefix != NULL);

    set_error(
        out_errmsg,
        "%s" "expected %s, found %s",
        prefix, lua_typename(L, expected_type), luaL_typename(L, pos)
    );
}

void set_curl_error(char **out_errmsg, CURLcode rc)
{
    set_error(out_errmsg, "libcurl error: %s", curl_easy_strerror(rc));
}
