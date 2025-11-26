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

#include "describe_lua_err.h"
#include <lua.h>
#include <lauxlib.h>
#include "libls/ls_assert.h"

void describe_lua_err(
    lua_State *L,
    int lretval,
    const char **out_err_kind,
    const char **out_err_msg)
{
    LS_ASSERT(lretval != 0);

    const char *kind;
    switch (lretval) {
    case LUA_ERRRUN:
        kind = "runtime error";
        break;
    case LUA_ERRSYNTAX:
        kind = "syntax error";
        break;
    case LUA_ERRMEM:
        kind = "out of memory";
        break;
    case LUA_ERRFILE:
        kind = "cannot read file with program code";
        break;
    case LUA_ERRERR:
        kind = "error while running error handler";
        break;
#ifdef LUA_ERRGCMM
    // Introduced in Lua 5.2 and removed in in Lua 5.4.
    case LUA_ERRGCMM:
        kind = "error while running __gc metamethod";
        break;
#endif
    default:
        kind = "(unknown Lua error code)";
    }

    const char *msg = lua_tostring(L, -1);
    if (!msg) {
        msg = "(error object cannot be converted to string)";
    }

    *out_err_kind = kind;
    *out_err_msg = msg;
}
