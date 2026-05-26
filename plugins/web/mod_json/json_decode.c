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

// It is actually located in ${CMAKE_CURRENT_BINARY_DIR}.
// CMakeLists.txt adds it to the "include directories" list.
#include "json_config.generated.h"

#if CJSON_FOUND_BY_PKG_CONFIG
#   include <cJSON.h>
#else
#   include <cjson/cJSON.h>
#endif

#include "libls/ls_panic.h"
#include "libls/ls_lua_madness.h"

enum { MAX_DEPTH = 200 };

typedef struct {
    JsonDecodeRefs refs;
    int flags;
    bool is_ok;
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
    lua_rawgeti(L, LUA_REGISTRYINDEX, lref); // L: ? table mt
    lua_setmetatable(L, -2); // L: ? table
}

void json_decode_reg_refs(lua_State *L, JsonDecodeRefs *out)
{
    out->lref_mt_arr  = mt_new(L, "is_array");
    out->lref_mt_dict = mt_new(L, "is_dict");
}

// Forward declaration
static bool convert(lua_State *L, cJSON *j, Params *params, int recur_lim);

static bool convert_array(lua_State *L, cJSON *j, Params *params, int recur_lim)
{
    int n = cJSON_GetArraySize(j);
    lua_createtable(L, n, 0); // L: table

    if (params->flags & JSON_DEC_MARK_ARRAYS_VS_DICT) {
        mt_set(L, params->refs.lref_mt_arr);
    }

    unsigned i = 1;
    for (cJSON *item = j->child; item; item = item->next) {
        if (!convert(L, item, params, recur_lim)) {
            return false;
        }
        // L: table value
        lua_rawseti(L, -2, i); // L: table
        ++i;
    }
    return true;
}

static bool convert_dict(lua_State *L, cJSON *j, Params *params, int recur_lim)
{
    // /cJSON_GetArraySize()/ it works for dicts too
    int n = cJSON_GetArraySize(j);
    lua_createtable(L, 0, n); // L: table

    if (params->flags & JSON_DEC_MARK_ARRAYS_VS_DICT) {
        mt_set(L, params->refs.lref_mt_dict);
    }

    for (cJSON *item = j->child; item; item = item->next) {
        if (!convert(L, item, params, recur_lim)) {
            return false;
        }
        // L: table value
        lua_setfield(L, -2, item->string); // L: table
    }
    return true;
}

static bool convert(lua_State *L, cJSON *j, Params *params, int recur_lim)
{
    if (!recur_lim--) {
        params->err_descr = "depth limit exceeded";
        return false;
    }

    // This should never happen. Still, let's do it, just to be safe.
    if (!lua_checkstack(L, 10)) {
        params->err_descr = "too many elements on Lua stack";
        return false;
    }

    if (cJSON_IsNull(j)) {
        if (params->flags & JSON_DEC_MARK_NULLS) {
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
        return convert_array(L, j, params, recur_lim);

    } else if (cJSON_IsObject(j)) {
        return convert_dict(L, j, params, recur_lim);

    } else {
        LS_MUST_BE_UNREACHABLE();
    }
}

static int x_convert(lua_State *L) /*__FATAL_IF_THROWS__*/
{
    cJSON *j = lua_touserdata(L, 1);
    Params *params = lua_touserdata(L, 2);

    bool is_ok = convert(L, j, params, MAX_DEPTH);
    params->is_ok = is_ok;
    if (!is_ok) {
        lua_settop(L, 0);
        lua_pushnil(L);
    }
    return 1;
}

bool json_decode(
        lua_State *L,
        const JsonDecodeRefs *refs,
        const char *input,
        int flags,
        char *errbuf,
        size_t nerrbuf)
{
    LS_ASSERT(input != NULL);
    LS_ASSERT(refs != NULL);

    if (!lua_checkstack(L, MAX_DEPTH + 10)) {
        snprintf(errbuf, nerrbuf, "Lua failed to allocate stack of required size");
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
        .refs = *refs,
        .flags = flags,
        .is_ok = false,
        .err_descr = NULL,
    };

    /*__OK__*/ lua_pushcfunction(L, x_convert);
    lua_pushlightuserdata(L, j);
    lua_pushlightuserdata(L, &params);
    ls_lua_madness_call_or_die(L, 2, 1);

    cJSON_Delete(j);

    if (!params.is_ok) {
        lua_pop(L, 1);
        snprintf(errbuf, nerrbuf, "%s", params.err_descr);
    }
    return params.is_ok;
}
