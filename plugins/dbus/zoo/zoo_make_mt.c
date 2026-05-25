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

#include "zoo_make_mt.h"

#include <lua.h>
#include <lauxlib.h>
#include "zoo_registry.h"

void zoo_make_mt(
        lua_State *L,
        const char *mt_name,
        const Zoo_RegistryEntry *registry)
{
    // L: ?
    luaL_newmetatable(L, mt_name); // L: ? mt

    lua_pushvalue(L, -1); // L: ? mt mt
    lua_setfield(L, -2, "__index"); // L: ? mt

    zoo_registry_register(L, registry); // L: ? mt

    lua_pop(L, 1); // L: ?
}
