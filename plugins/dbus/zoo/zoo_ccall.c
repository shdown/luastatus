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

#include "zoo_ccall.h"

bool zoo_ccall(lua_State *L, int nargs, int nresults, lua_CFunction f, void *ud)
{
    // See 'DOCS/c_notes/lua_cfuncs.md' in the root of the repo for what __OK__ means.

    // L: ? args
    lua_pushlightuserdata(L, ud); // L: ? args ud
    /*__OK__*/ lua_pushcfunction(L, f); // L: ? args ud f
    int total = nargs + 1;
    lua_insert(L, lua_gettop(L) - total); // L: ? f args ud
    int rc = lua_pcall(L, total, nresults, 0);
    return rc == 0;
}
