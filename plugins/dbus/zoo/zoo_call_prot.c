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

#include "zoo_call_prot.h"
#include <stdbool.h>
#include <lua.h>

bool zoo_call_prot(lua_State *L, int nargs, int nresults, lua_CFunction f, void *f_ud)
{
    // L: ? args
    lua_pushlightuserdata(L, f_ud); // L: ? args ud
    lua_pushcclosure(L, f, 1); // L: ? args func
    lua_insert(L, -(nargs + 1)); // L: ? func args
    return lua_pcall(L, nargs, nresults, 0) == 0;
}
