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

#include "zoo_registry.h"
#include <lua.h>

void zoo_registry_register(
        lua_State *L,
        const Zoo_RegistryEntry *registry)
{
    // See 'DOCS/c_notes/lua_cfuncs.md' in the root of the repo for what __OK__ means.

    // L: ? table
    for (const Zoo_RegistryEntry *e = registry; e->name; ++e) {
        // L: ? table
        /*__OK__*/ lua_pushcfunction(L, e->f); // L: ? table func
        lua_setfield(L, -2, e->name); // L: ? table
    }
    // L: ? table
}

void zoo_registry_register_with_upvalue(
        lua_State *L,
        const Zoo_RegistryEntry *registry)
{
    // See 'DOCS/c_notes/lua_cfuncs.md' in the root of the repo for what __OK__ means.

    // L: ? table upvalue
    for (const Zoo_RegistryEntry *e = registry; e->name; ++e) {
        // L: ? table upvalue
        lua_pushvalue(L, -1); // L: ? table upvalue upvalue
        /*__OK__*/ lua_pushcclosure(L, e->f, 1); // L: ? table upvalue func
        lua_setfield(L, -3, e->name); // L: ? table upvalue
    }
    // L: ? table upvalue
}
