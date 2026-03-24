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

#include "zoo_checkudata.h"
#include <lua.h>
#include <lauxlib.h>
#include "libls/ls_panic.h"

void *zoo_checkudata(lua_State *L, int pos, const char *tname, const char *what)
{
    // L: ?
    void *ud = lua_touserdata(L, pos);
    if (!ud) {
        goto error;
    }
    if (!lua_getmetatable(L, pos)) {
        goto error;
    }
    // L: ? actual_mt
    lua_getfield(L, LUA_REGISTRYINDEX, tname);
    // L: ? actual_mt expected_mt
    if (!lua_rawequal(L, -1, -2)) {
        goto error;
    }
    lua_pop(L, 2); // L: ?
    return ud;
error:
    (void) luaL_error(L, "%s: is not a '%s' userdata value", what, tname);
    LS_MUST_BE_UNREACHABLE();
}
