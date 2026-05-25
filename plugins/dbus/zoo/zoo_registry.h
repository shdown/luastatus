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

#pragma once

#include <lua.h>

typedef struct {
    const char *name;
    lua_CFunction f;
} Zoo_RegistryEntry;

#define ZOO_REG_ENT(name, f) {(name), (f)}

void zoo_registry_register(
        lua_State *L,
        const Zoo_RegistryEntry *registry);

void zoo_registry_register_with_upvalue(
        lua_State *L,
        const Zoo_RegistryEntry *registry);
