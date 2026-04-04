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

#include "mod_json.h"
#include <stdbool.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include "libsafe/safev.h"
#include "json_decode.h"
#include "json_encode_str.h"
#include "json_encode_num.h"

static inline bool getbool(lua_State *L, int arg)
{
    if (lua_isnoneornil(L, arg)) {
        return false;
    }
    luaL_checktype(L, arg, LUA_TBOOLEAN);
    return lua_toboolean(L, arg);
}

static int l_json_decode(lua_State *L)
{
    enum { MAX_DEPTH = 100 };

    const char *input = luaL_checkstring(L, 1);
    bool mark_arrays_vs_dicts = getbool(L, 2);
    bool mark_nulls = getbool(L, 3);

    int flags = 0;
    if (mark_arrays_vs_dicts) {
        flags |= JSON_DEC_MARK_ARRAYS_VS_DICT;
    }
    if (mark_nulls) {
        flags |= JSON_DEC_MARK_NULLS;
    }

    char errbuf[256];

    bool is_ok = json_decode(L, input, MAX_DEPTH, flags, errbuf, sizeof(errbuf));

    if (is_ok) {
        return 1;
    } else {
        lua_settop(L, 0);
        lua_pushnil(L);
        lua_pushstring(L, errbuf);
        return 2;
    }
}

static void append_to_lua_buf_callback(void *ud, SAFEV v)
{
    luaL_Buffer *b = ud;
    luaL_addlstring(b, SAFEV_ptr_UNSAFE(v), SAFEV_len(v));
}

static int l_json_encode_str(lua_State *L)
{
    size_t ns;
    const char *s = luaL_checklstring(L, -1, &ns);

    luaL_Buffer b;
    luaL_buffinit(L, &b);
    json_encode_str(append_to_lua_buf_callback, &b, SAFEV_new_UNSAFE(s, ns));

    luaL_pushresult(&b); // L: result
    return 1;
}

static int l_json_encode_num(lua_State *L)
{
    double d = luaL_checknumber(L, 1);
    char *res = json_encode_num(d);
    if (res) {
        lua_pushstring(L, res);
    } else {
        lua_pushnil(L);
    }
    free(res);
    return 1;
}

void mod_json_register_funcs(lua_State *L)
{
#define REG(func, name) (lua_pushcfunction(L, (func)), lua_setfield((L), -2, (name)))

    REG(l_json_decode, "json_decode");
    REG(l_json_encode_str, "json_encode_str");
    REG(l_json_encode_num, "json_encode_num");

#undef REG
}
