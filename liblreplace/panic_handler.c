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

#include "panic_handler.h"

#include <lua.h>
#include <stdio.h>
#include <stdlib.h>

static int panic(lua_State *L) /*__TERMINATES_PROGRAM__*/
{
    const char *msg;
    if (lua_type(L, -1) == LUA_TSTRING) {
        msg = lua_tostring(L, -1);
    } else {
        msg = "error object is not a string";
    }
    fprintf(stderr, "PANIC: unprotected error in call to Lua API (%s)\n", msg);
    fflush(stderr);
    fflush(stdout);
    abort();
    // unreachable
}

void liblreplace_panic_handler_install(lua_State *L)
{
    lua_atpanic(L, panic);
}
