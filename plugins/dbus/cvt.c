/*
 * Copyright (C) 2015-2026  luastatus developers
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

#include "cvt.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <stdbool.h>
#include <glib/gtypes.h>
#include <lua.h>
#include "libls/ls_compdep.h"
#include "libls/ls_panic.h"
#include "libls/ls_lua_compat.h"
#include "libls/ls_lua_madness.h"

enum { MAX_DEPTH = 200 };

static int l_special_object(lua_State *L) /*__THROWABLE__*/
{
    lua_pushvalue(L, lua_upvalueindex(1)); // L: upvalue1
    if (lua_isnil(L, -1)) {
        lua_pushvalue(L, lua_upvalueindex(2)); // L: upvalue1 upvalue2
        return 2;
    } else {
        // L: upvalue1
        return 1;
    }
}

static void push_special_object(lua_State *L, const char *s, bool is_error)
{
    // See 'DOCS/c_notes/lua_cfuncs.md' in the root of the repo for what __OK__ means.

    int num_upvalues = 1;
    if (is_error) {
        lua_pushnil(L);
        num_upvalues = 2;
    }
    lua_pushstring(L, s);
    /*__OK__*/ lua_pushcclosure(L, l_special_object, num_upvalues);
}

// forward declaration
static void push_gvariant(lua_State *L, GVariant *var, unsigned recurlim);

static void on_recur_lim(lua_State *L)
{
    push_special_object(L, "depth limit exceeded", true);
}

static void on_checkstack_failure(lua_State *L)
{
    push_special_object(L, "out of memory", true);
}

static inline void push_gvariant_strlike(lua_State *L, GVariant *var)
{
    gsize ns;
    const gchar *s = g_variant_get_string(var, &ns);
    lua_pushlstring(L, s, ns);
}

static void push_gvariant_iterable(lua_State *L, GVariant *var, unsigned recurlim)
{
    GVariantIter iter;
    g_variant_iter_init(&iter, var);

    size_t n = g_variant_iter_n_children(&iter);
    if (n > (size_t) LS_LUA_MAXI) {
        push_special_object(L, "array would be too big", true);
        return;
    }
    lua_createtable(L, ls_lua_num_prealloc(n), 0); // L: table

    GVariant *elem;
    for (size_t i = 1; (elem = g_variant_iter_next_value(&iter)); ++i) {
        push_gvariant(L, elem, recurlim); // L: table value
        g_variant_unref(elem);
        lua_rawseti(L, -2, i); // L: table
    }
}

static inline LS_ATTR_PRINTF(2, 3)
void push_small_fstr(lua_State *L, const char *fmt, ...)
{
    char buf[32];
    va_list vl;
    va_start(vl, fmt);
    vsnprintf(buf, sizeof(buf), fmt, vl);
    va_end(vl);
    lua_pushstring(L, buf);
}

static void push_gvariant(lua_State *L, GVariant *var, unsigned recurlim)
{
    LS_ASSERT(var != NULL);

    if (!recurlim--) {
        on_recur_lim(L);
        return;
    }

    // This should never happen. Still, let's do it, just to be safe.
    if (!lua_checkstack(L, 10)) {
        on_checkstack_failure(L);
        return;
    }

    switch (g_variant_classify(var)) {
    case G_VARIANT_CLASS_BOOLEAN:
        lua_pushboolean(L, !!g_variant_get_boolean(var));
        break;

    case G_VARIANT_CLASS_BYTE:
        push_small_fstr(L, "%" PRIu8, (uint8_t) g_variant_get_byte(var));
        break;

    case G_VARIANT_CLASS_INT16:
        push_small_fstr(L, "%" PRIi16, (int16_t) g_variant_get_int16(var));
        break;

    case G_VARIANT_CLASS_UINT16:
        push_small_fstr(L, "%" PRIu16, (uint16_t) g_variant_get_uint16(var));
        break;

    case G_VARIANT_CLASS_INT32:
        push_small_fstr(L, "%" PRIi32, (int32_t) g_variant_get_int32(var));
        break;

    case G_VARIANT_CLASS_UINT32:
        push_small_fstr(L, "%" PRIu32, (uint32_t) g_variant_get_uint32(var));
        break;

    case G_VARIANT_CLASS_INT64:
        push_small_fstr(L, "%" PRIi64, (int64_t) g_variant_get_int64(var));
        break;

    case G_VARIANT_CLASS_UINT64:
        push_small_fstr(L, "%" PRIu64, (uint64_t) g_variant_get_uint64(var));
        break;

    case G_VARIANT_CLASS_DOUBLE:
        lua_pushnumber(L, g_variant_get_double(var));
        break;

    case G_VARIANT_CLASS_STRING:
    case G_VARIANT_CLASS_OBJECT_PATH:
    case G_VARIANT_CLASS_SIGNATURE:
        push_gvariant_strlike(L, var);
        break;

    case G_VARIANT_CLASS_VARIANT:
        {
            GVariant *boxed = g_variant_get_variant(var);
            push_gvariant(L, boxed, recurlim);
            g_variant_unref(boxed);
        }
        break;

    case G_VARIANT_CLASS_ARRAY:
    case G_VARIANT_CLASS_TUPLE:
    case G_VARIANT_CLASS_DICT_ENTRY:
        push_gvariant_iterable(L, var, recurlim);
        break;

    case G_VARIANT_CLASS_HANDLE:
        push_special_object(L, "handle", false);
        break;

    default:
        push_special_object(L, "unknown", false);
        break;
    }
}

static int x_cvt(lua_State *L) /*__FATAL_IF_THROWS__*/
{
    GVariant *var = lua_touserdata(L, 1);

    if (!var) {
        lua_pushnil(L);
        return 1;
    }

    if (lua_checkstack(L, MAX_DEPTH + 10)) {
        push_gvariant(L, var, MAX_DEPTH);
    } else {
        on_checkstack_failure(L);
    }
    return 1;
}

void cvt(lua_State *L, GVariant *var)
{
    // See 'DOCS/c_notes/lua_cfuncs.md' in the root of the repo for what __OK__ means.

    /*__OK__*/ lua_pushcfunction(L, x_cvt);
    lua_pushlightuserdata(L, var);
    ls_lua_madness_call_or_die(L, 1, 1);
}
