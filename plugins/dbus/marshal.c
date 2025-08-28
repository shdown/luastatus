/*
 * Copyright (C) 2015-2020  luastatus developers
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

#include "marshal.h"

#include "libls/compdep.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <inttypes.h>
#include <glib/gtypes.h>
#include <lua.h>

static int l_special_object(lua_State *L)
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

// forward declaration
static void push_gvariant(lua_State *L, GVariant *var, unsigned recurlim);

static void on_recur_lim(lua_State *L)
{
    lua_pushnil(L); // L: nil
    lua_pushstring(L, "depth limit exceeded"); // L: nil str
    lua_pushcclosure(L, l_special_object, 2); // L: closure
}

static inline void push_gvariant_strlike(lua_State *L, GVariant *var)
{
    gsize ns;
    const gchar *s = g_variant_get_string(var, &ns);
    lua_pushlstring(L, s, ns);
}

static void push_gvariant_iterable(lua_State *L, GVariant *var, unsigned recurlim)
{
    if (!recurlim--) {
        on_recur_lim(L);
        return;
    }

    lua_newtable(L); // L: table

    GVariantIter iter;
    g_variant_iter_init(&iter, var);
    GVariant *elem;
    for (unsigned i = 1; (elem = g_variant_iter_next_value(&iter)); ++i) {
        push_gvariant(L, elem, recurlim); // L: table value
        lua_rawseti(L, -2, i); // L: table
        g_variant_unref(elem);
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
    if (!recurlim--) {
        on_recur_lim(L);
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

    case G_VARIANT_CLASS_MAYBE:
        {
            GVariant *maybe = g_variant_get_maybe(var);
            if (maybe) {
                push_gvariant(L, maybe, recurlim);
                g_variant_unref(maybe);
            } else {
                lua_pushstring(L, "nothing"); // L: str
                lua_pushcclosure(L, l_special_object, 1); // L: closure
            }
        }
        break;

    case G_VARIANT_CLASS_ARRAY:
    case G_VARIANT_CLASS_TUPLE:
    case G_VARIANT_CLASS_DICT_ENTRY:
        push_gvariant_iterable(L, var, recurlim);
        break;

    case G_VARIANT_CLASS_HANDLE:
    default:
        {
            const GVariantType *type = g_variant_get_type(var);
            const gchar *s = g_variant_type_peek_string(type);
            gsize ns = g_variant_type_get_string_length(type);
            lua_pushlstring(L, s, ns); // L: str
            lua_pushcclosure(L, l_special_object, 1); // L: closure
        }
        break;
    }
}

void marshal(lua_State *L, GVariant *var)
{
    if (!var) {
        lua_pushnil(L);
        return;
    }

    if (lua_checkstack(L, 210)) {
        push_gvariant(L, var, 200);
    } else {
        lua_pushnil(L); // L: nil
        lua_pushstring(L, "out of memory"); // L: nil str
        lua_pushcclosure(L, l_special_object, 2); // L: closure
    }
}
