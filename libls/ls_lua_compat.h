/*
 * Copyright (C) 2015-2025  luastatus developers
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

#ifndef ls_lua_compat_h_
#define ls_lua_compat_h_

#include <lua.h>
#include <lauxlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include "ls_compdep.h"

#if LUA_VERSION_NUM >= 504
#   define ls_lua_pushfail(L_) luaL_pushfail(L_)
#else
#   define ls_lua_pushfail(L_) lua_pushnil(L_)
#endif

#include "ls_compdep.h"

#if LUA_VERSION_NUM >= 504
#   define ls_lua_pushfail(L_) luaL_pushfail(L_)
#else
#   define ls_lua_pushfail(L_) lua_pushnil(L_)
#endif

LS_INHEADER bool ls_lua_is_lua51(lua_State *L)
{
#if LUA_VERSION_NUM >= 502
    (void) L;
    return false;
#else
    // LuaJIT, when compiled with -DLUAJIT_ENABLE_LUA52COMPAT, still defines
    // LUA_VERSION_NUM to 501, but syntax parser, library functions and some
    // aspect of language work as if it was Lua 5.2.

    // L: ?
    lua_getglobal(L, "rawlen"); // L: ? rawlen
    bool ret = lua_isnil(L, -1);
    lua_pop(L, 1); // L: ?
    return ret;
#endif
}

LS_INHEADER size_t ls_lua_array_len(lua_State *L, int pos)
{
#if LUA_VERSION_NUM <= 501
    return lua_objlen(L, pos);
#else
    return lua_rawlen(L, pos);
#endif
}

LS_INHEADER void ls_lua_geti(lua_State *L, int pos, size_t i)
{
#if LUA_VERSION_NUM <= 502
    // L: ?
    lua_pushinteger(L, i); // L: ? idx
    lua_gettable(L, pos < 0 ? pos - 1 : pos); // L: ? value
#else
    lua_geti(L, pos, i);
#endif
}

#ifdef LUA_MAXINTEGER
# define LS_LUA_MAXI \
    (LUA_MAXINTEGER > (SIZE_MAX - 1) ? (SIZE_MAX - 1) : LUA_MAXINTEGER)
#else
# define LS_LUA_MAXI INT_MAX
#endif

#endif
