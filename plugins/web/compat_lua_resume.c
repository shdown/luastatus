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

#include "compat_lua_resume.h"
#include <lua.h>

#if LUA_VERSION_NUM >= 504
static int do_resume(lua_State *L, lua_State *from, int nargs)
{
    int unused;
    return lua_resume(L, from, nargs, &unused);
}

#elif LUA_VERSION_NUM >= 502
static int do_resume(lua_State *L, lua_State *from, int nargs)
{
    return lua_resume(L, from, nargs);
}

#else
static int do_resume(lua_State *L, lua_State *from, int nargs)
{
    (void) from;
    return lua_resume(L, nargs);
}

#endif

int compat_lua_resume(lua_State *L, lua_State *from, int nargs, int *nresults)
{
    int old_top = lua_gettop(L);
    int rc = do_resume(L, from, nargs);
    if (rc == LUA_YIELD || rc == 0) {
        *nresults = lua_gettop(L) - (old_top - (nargs + 1));
    }
    return rc;
}
