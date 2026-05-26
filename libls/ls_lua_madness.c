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

#include "ls_lua_madness.h"
#include <lua.h>
#include <stddef.h>
#include <string.h>
#include "ls_panic.h"

typedef struct {
    const char *b;
    size_t nb;
} Pushlstr_Arg;

static int x_pushlstr(lua_State *L) /*__FATAL_IF_THROWS__*/
{
    const Pushlstr_Arg *arg = lua_touserdata(L, 1);
    if (arg) {
        lua_pushlstring(L, arg->b, arg->nb);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

void ls_lua_madness_pushlstr(lua_State *L, const char *b, size_t nb)
{
    // See 'DOCS/c_notes/lua_cfuncs.md' in the root of the repo for what __OK__ means.

    /*__OK__*/ lua_pushcfunction(L, x_pushlstr);

    Pushlstr_Arg arg = {.b = b, .nb = nb};
    lua_pushlightuserdata(L, &arg);

    ls_lua_madness_call_or_die(L, 1, 1);
}

void ls_lua_madness_pushstr(lua_State *L, const char *s)
{
    // See 'DOCS/c_notes/lua_cfuncs.md' in the root of the repo for what __OK__ means.

    /*__OK__*/ lua_pushcfunction(L, x_pushlstr);

    Pushlstr_Arg arg;
    if (s) {
        arg = (Pushlstr_Arg) {.b = s, .nb = strlen(s)};
        lua_pushlightuserdata(L, &arg);
    } else {
        lua_pushlightuserdata(L, NULL);
    }

    ls_lua_madness_call_or_die(L, 1, 1);
}

void ls_lua_madness_call_or_die(lua_State *L, int nargs, int nresults)
{
    int rc = lua_pcall(L, nargs, nresults, 0);
    if (rc != 0) {
        const char *msg;
        if (rc == LUA_ERRMEM) {
            msg = "out of memory (reported from Lua)";
        } else {
            msg = "error thrown out of a lua_CFunction that must never throw";
        }
        LS_PANIC(msg);
    }
}
