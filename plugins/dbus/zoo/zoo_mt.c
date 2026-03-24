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

#include "zoo_mt.h"

#include <lua.h>
#include <lauxlib.h>

void zoo_mt_begin(lua_State *L, const char *mt_name)
{
    // L: ?
    luaL_newmetatable(L, mt_name); // L: ? mt

    lua_pushvalue(L, -1); // L: ? mt mt
    lua_setfield(L, -2, "__index"); // L: ? mt
}

void zoo_mt_add_method(lua_State *L, const char *name, lua_CFunction f)
{
    lua_pushcfunction(L, f); // L: ? mt f
    lua_setfield(L, -2, name); // L: ? mt
}

void zoo_mt_end(lua_State *L)
{
    // L: ? mt
    lua_pop(L, 1); // L: ?
}
