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

#include "json_decode.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <lua.h>
#include <lauxlib.h>
#include <cJSON.h>
#include "libls/ls_panic.h"

typedef struct {
    int recur_lim;
    int lref_mt_array;
    int lref_mt_dict;
    bool mark_nulls;
    const char *err_descr;
} Params;

static int mt_new(lua_State *L, const char *field)
{
    // L: ?
    lua_createtable(L, 0, 1); // L: ? mt
    lua_pushboolean(L, 1); // L: ? mt boolean
    lua_setfield(L, -2, field); // L: ? mt
    return luaL_ref(L, LUA_REGISTRYINDEX); // L: ?
}

static void mt_set(lua_State *L, int lref)
{
    // L: ? mt
    if (lref == LUA_REFNIL) {
        return;
    }
    lua_rawgeti(L, LUA_REGISTRYINDEX, lref); // L: ? table mt
    lua_setmetatable(L, -2); // L: ? table
}

static void mt_unref(lua_State *L, int lref)
{
    if (lref == LUA_REFNIL) {
        return;
    }
    luaL_unref(L, LUA_REGISTRYINDEX, lref);
}

// Forward declaration
static bool convert(lua_State *L, cJSON *j, Params *params);

static bool convert_array(lua_State *L, cJSON *j, Params *params)
{
    int n = cJSON_GetArraySize(j);
    lua_createtable(L, n, 0); // L: table

    mt_set(L, params->lref_mt_array);

    unsigned i = 1;
    for (cJSON *item = j->child; item; item = item->next) {
        if (!convert(L, item, params)) {
            return false;
        }
        // L: table value
        lua_rawseti(L, -2, i); // L: table
        ++i;
    }
    return true;
}

static bool convert_dict(lua_State *L, cJSON *j, Params *params)
{
    // /cJSON_GetArraySize()/ it works for dicts too
    int n = cJSON_GetArraySize(j);
    lua_createtable(L, 0, n); // L: table

    mt_set(L, params->lref_mt_dict);

    for (cJSON *item = j->child; item; item = item->next) {
        if (!convert(L, item, params)) {
            return false;
        }
        // L: table value
        lua_setfield(L, -2, item->string); // L: table
    }
    return true;
}

static bool convert(lua_State *L, cJSON *j, Params *params)
{
    if (!params->recur_lim--) {
        params->err_descr = "depth limit exceeded";
        return false;
    }
    if (!lua_checkstack(L, 10)) {
        params->err_descr = "too many elements on Lua stack";
        return false;
    }

    if (cJSON_IsNull(j)) {
        if (params->mark_nulls) {
            lua_pushlightuserdata(L, NULL);
        } else {
            lua_pushnil(L);
        }
        return true;

    } else if (cJSON_IsTrue(j)) {
        lua_pushboolean(L, 1);
        return true;

    } else if (cJSON_IsFalse(j)) {
        lua_pushboolean(L, 0);
        return true;

    } else if (cJSON_IsNumber(j)) {
        lua_pushnumber(L, j->valuedouble);
        return true;

    } else if (cJSON_IsString(j)) {
        lua_pushstring(L, j->valuestring);
        return true;

    } else if (cJSON_IsArray(j)) {
        return convert_array(L, j, params);

    } else if (cJSON_IsObject(j)) {
        return convert_dict(L, j, params);

    } else {
        LS_MUST_BE_UNREACHABLE();
    }
}

bool json_decode(lua_State *L, const char *input, int max_depth, int flags, char *errbuf, size_t nerrbuf)
{
    LS_ASSERT(input != NULL);

    if (!lua_checkstack(L, max_depth)) {
        snprintf(errbuf, nerrbuf, "Lua failed to allocate stack of size %d", max_depth);
        return false;
    }

    if (strlen(input) > (INT_MAX - 16)) {
        snprintf(errbuf, nerrbuf, "JSON payload is too large");
        return false;
    }

    // /cJSON_GetErrorPtr/ is not thread-safe, so we use /cJSON_ParseWithOpts/.
    const char *err_ptr;
    cJSON *j = cJSON_ParseWithOpts(input, &err_ptr, /*require_null_terminate=*/ 1);
    if (!j) {
        snprintf(errbuf, nerrbuf, "JSON parse error at byte %d", (int) (err_ptr - input));
        return false;
    }

    Params params = {
        .recur_lim = max_depth,
        .lref_mt_array = LUA_REFNIL,
        .lref_mt_dict = LUA_REFNIL,
        .mark_nulls = false,
        .err_descr = NULL,
    };
    if (flags & JSON_DEC_MARK_ARRAYS_VS_DICT) {
        params.lref_mt_array = mt_new(L, "is_array");
        params.lref_mt_dict = mt_new(L, "is_dict");
    }
    if (flags & JSON_DEC_MARK_NULLS) {
        params.mark_nulls = true;
    }

    bool is_ok = convert(L, j, &params);

    mt_unref(L, params.lref_mt_array);
    mt_unref(L, params.lref_mt_dict);

    if (!is_ok) {
        snprintf(errbuf, nerrbuf, "%s", params.err_descr);
    }
    cJSON_Delete(j);
    return is_ok;
}
