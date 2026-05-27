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

#include "osstuff.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <lua.h>
#include <lauxlib.h>

#if LUA_VERSION_NUM == 501
static int l_os_exit(lua_State *L) /*__THROWABLE__*/
{
    int status = luaL_optint(L, 1, EXIT_SUCCESS);

    fflush(stderr);
    fflush(stdout);
    _exit(status);
}
#elif LUA_VERSION_NUM >= 502
static int l_os_exit(lua_State *L) /*__THROWABLE__*/
{
    int status;
    if (!lua_isnone(L, 1) && lua_isboolean(L, 1)) {
        status = lua_toboolean(L, 1) ? EXIT_SUCCESS : EXIT_FAILURE;
    } else {
        status = luaL_optinteger(L, 1, EXIT_SUCCESS);
    }

    if (!lua_isnone(L, 2)) {
        if (lua_toboolean(L, 2)) {
            lua_close(L);
        }
    }

    fflush(stderr);
    fflush(stdout);
    _exit(status);
}
#else
# error "Unsupported LUA_VERSION_NUM."
#endif

static int l_os_setlocale(lua_State *L) /*__THROWABLE__*/
{
    return luaL_error(L, "os.setlocale() is not supported in luastatus");
}

void liblreplace_osstuff_inject(lua_State *L)
{
    // See 'DOCS/c_notes/lua_cfuncs.md' in the root of the repo for what __OK__ means.

    // L: ?
    lua_getglobal(L, "os"); // L: ? os

    /*__OK__*/ lua_pushcfunction(L, l_os_exit); // L: ? os func
    lua_setfield(L, -2, "exit"); // L: ? os

    /*__OK__*/ lua_pushcfunction(L, l_os_setlocale); // L: ? os func
    lua_setfield(L, -2, "setlocale"); // L: ? os

    lua_pop(L, 1); // L: ?
}
