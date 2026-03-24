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

#include "load_lualib.h"
#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include "include/sayf_macros.h"
#include "include/plugin_data_v1.h"

static void report_and_pop_error(LuastatusPluginData *pd, lua_State *L, int rc)
{
    const char *msg = lua_tostring(L, -1);
    if (!msg) {
        msg = "(error object cannot be converted to string)";
    }
    LS_ERRF(pd, "cannot load lualib.lua: [%d] %s", rc, msg);

    lua_pop(L, 1); // L: ?
}

bool load_lualib(LuastatusPluginData *pd, lua_State *L)
{
    static const char *LUA_SOURCE =
#include "lualib.generated.inc"
    ;
    int rc;

    // L: ? table
    rc = luaL_loadbuffer(L, LUA_SOURCE, strlen(LUA_SOURCE), "<bundled lualib.lua>");
    if (rc != 0) {
        // L: ? table error
        report_and_pop_error(pd, L, rc);
        // L: ? table
        return false;
    }
    // L: ? table chunk
    lua_pushvalue(L, -2); // L: ? table chunk table

    rc = lua_pcall(L, 1, 0, 0);
    if (rc != 0) {
        // L: ? table error
        report_and_pop_error(pd, L, rc);
        // L: ? table
        return false;
    }
    // L: ? table

    return true;
}
