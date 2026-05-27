/*
 * Copyright (C) 2015-2026  luastatus developers
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

LS_INHEADER size_t ls_lua_array_len(lua_State *L, int pos)
{
#if LUA_VERSION_NUM <= 501
    return lua_objlen(L, pos);
#else
    return lua_rawlen(L, pos);
#endif
}

LS_INHEADER void *ls_lua_newud(lua_State *L, size_t sz)
{
    // We're not sure if lua_newuserdata()/lua_newuserdatauv() handle zero size.
    // So replace zero size with 1 byte.
    if (!sz) {
        sz = 1;
    }

#if LUA_VERSION_NUM >= 504
    return lua_newuserdatauv(L, sz, 1);
#else
    return lua_newuserdata(L, sz);
#endif
}

#ifdef LUA_MAXINTEGER
# define LS_LUA_MAXI \
    (LUA_MAXINTEGER > (SIZE_MAX - 1) ? (SIZE_MAX - 1) : LUA_MAXINTEGER)
#else
# define LS_LUA_MAXI INT_MAX
#endif

LS_INHEADER int ls_lua_num_prealloc(size_t n)
{
    return n < INT_MAX ? n : INT_MAX;
}
