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

#include "mod_urlencode.h"
#include <stdbool.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include "libsafe/safev.h"
#include "libsafe/mut_safev.h"
#include "urlencode.h"
#include "urldecode.h"
#include "libls/ls_alloc_utils.h"
#include "libls/ls_lua_compat.h"

static inline bool getbool(lua_State *L, int arg)
{
    if (lua_isnoneornil(L, arg)) {
        return false;
    }
    luaL_checktype(L, arg, LUA_TBOOLEAN);
    return lua_toboolean(L, arg);
}

static int l_urlencode(lua_State *L) /*__THROWABLE__*/
{
    size_t ns;
    const char *s = luaL_checklstring(L, 1, &ns);
    bool plus_notation = getbool(L, 2);

    SAFEV src = SAFEV_new_UNSAFE(s, ns);

    size_t nres = urlencode_size(src, plus_notation);
    if (nres == (size_t) -1) {
        ls_oom();
    }

    char *buf = ls_lua_newud(L, nres); // L: ? ud

    urlencode(src, MUT_SAFEV_new_UNSAFE(buf, nres), plus_notation);

    lua_pushlstring(L, buf, nres); // L: ? ud str

    return 1;
}

static int l_urldecode(lua_State *L) /*__THROWABLE__*/
{
    size_t ns;
    const char *s = luaL_checklstring(L, 1, &ns);

    SAFEV src = SAFEV_new_UNSAFE(s, ns);

    size_t nres = urldecode_size(src);

    char *buf = ls_lua_newud(L, nres); // L: ? ud

    if (urldecode(src, MUT_SAFEV_new_UNSAFE(buf, nres))) {
        lua_pushlstring(L, buf, nres); // L: ? ud str
    } else {
        lua_pushnil(L); // L: ? ud nil
    }
    return 1;
}

void mod_urlencode_register_funcs(lua_State *L)
{
    // See 'DOCS/c_notes/lua_cfuncs.md' in the root of the repo for what __OK__ means.

    /*__OK__*/ lua_pushcfunction(L, l_urlencode);
    lua_setfield(L, -2, "urlencode");

    /*__OK__*/ lua_pushcfunction(L, l_urldecode);
    lua_setfield(L, -2, "urldecode");
}
