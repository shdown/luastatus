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

#include "zoo_uncvt_type.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <lua.h>
#include <lauxlib.h>
#include <glib.h>
#include "libls/ls_alloc_utils.h"
#include "libls/ls_panic.h"
#include "libls/ls_lua_compat.h"
#include "zoo_call_prot.h"
#include "zoo_checkudata.h"
#include "zoo_mt.h"

typedef struct {
    // This is an owning handle, not a borrow.
    // May be NULL, which means it's already been "finalized" (garbage-collected).
    GVariantType *t;
} Tobj;

static const char *MT_NAME = "io.shdown.luastatus.plugin.dbus.official.Type";

static Tobj *fetch_tobj(lua_State *L, int pos, const char *what)
{
    return zoo_checkudata(L, pos, MT_NAME, what);
}

// Returns a borrow.
static const GVariantType *fetch_gtype_borrow(lua_State *L, int pos, const char *what)
{
    Tobj *tobj = fetch_tobj(L, pos, what);
    if (!tobj->t) {
        (void) luaL_error(L, "this type object has already been finalized");
        LS_MUST_BE_UNREACHABLE();
    }
    return tobj->t;
}

// /t/ must be an owning handle, which then gets "stolen".
static void make_tobj_steal(lua_State *L, GVariantType *t)
{
    LS_ASSERT(t != NULL);

    // L: ?
    Tobj *tobj = lua_newuserdata(L, sizeof(Tobj)); // L: ? ud
    tobj->t = t;

    luaL_getmetatable(L, MT_NAME); // L: ? ud mt
    lua_setmetatable(L, -2); // L: ? ud
}

// /t/ is a borrow, which gets copied.
static void make_tobj_copy(lua_State *L, const GVariantType *t)
{
    LS_ASSERT(t != NULL);
    make_tobj_steal(L, g_variant_type_copy(t));
}

static int tobj_gc(lua_State *L)
{
    Tobj *tobj = fetch_tobj(L, 1, "argument #1");
    if (tobj->t) {
        g_variant_type_free(tobj->t);
        tobj->t = NULL;
    }
    return 0;
}

static int l_equals_to(lua_State *L)
{
    // Both /t1/ and /t2/ are borrowed (STACK).
    const GVariantType *t1 = fetch_gtype_borrow(L, 1, "argument #1");
    const GVariantType *t2 = fetch_gtype_borrow(L, 2, "argument #2");

    lua_pushboolean(L, g_variant_type_equal(t1, t2));
    return 1;
}

static int l_get_type_string(lua_State *L)
{
    // /t/ is borrowed (STACK).
    const GVariantType *t = fetch_gtype_borrow(L, 1, "argument #1");

    size_t ns = g_variant_type_get_string_length(t);
    const char *s = g_variant_type_peek_string(t);

    lua_pushlstring(L, s, ns);
    return 1;
}

static int l_get_category(lua_State *L)
{
    // /t/ is borrowed (STACK).
    const GVariantType *t = fetch_gtype_borrow(L, 1, "argument #1");

    if (g_variant_type_is_basic(t) || g_variant_type_is_variant(t)) {
        lua_pushstring(L, "simple");
    } else if (g_variant_type_is_array(t)) {
        lua_pushstring(L, "array");
    } else if (g_variant_type_is_dict_entry(t)) {
        lua_pushstring(L, "dict_entry");
    } else if (g_variant_type_is_tuple(t)) {
        lua_pushstring(L, "tuple");
    } else {
        return luaL_argerror(L, 1, "unknown type");
    }
    return 1;
}

static int l_is_basic(lua_State *L)
{
    // /t/ is borrowed (STACK).
    const GVariantType *t = fetch_gtype_borrow(L, 1, "argument #1");

    lua_pushboolean(L, !!g_variant_type_is_basic(t));
    return 1;
}

static int l_get_item_types(lua_State *L)
{
    // /t/ is borrowed (STACK).
    const GVariantType *t = fetch_gtype_borrow(L, 1, "argument #1");

    if (!g_variant_type_is_tuple(t)) {
        return luaL_argerror(L, 1, "is not a tuple type");
    }

    // L: ?
    size_t n = g_variant_type_n_items(t);
    if (n > (size_t) LS_LUA_MAXI) {
        return luaL_error(L, "tuple type has too many elements");
    }
    lua_createtable(L, ls_lua_num_prealloc(n), 0); // L: ? arr

    size_t i = 1;
    for (
        const GVariantType *item_t = g_variant_type_first(t);
        item_t;
        item_t = g_variant_type_next(item_t))
    {
        // L: ? arr
        make_tobj_copy(L, item_t); // L: ? arr type
        lua_rawseti(L, -2, i); // L: ? arr
        ++i;
    }
    return 1;
}

static int l_get_elem_type(lua_State *L)
{
    // /t/ is borrowed (STACK).
    const GVariantType *t = fetch_gtype_borrow(L, 1, "argument #1");

    if (!g_variant_type_is_array(t)) {
        return luaL_argerror(L, 1, "is not an array type");
    }

    make_tobj_copy(L, g_variant_type_element(t));
    return 1;
}

static int l_get_kv_types(lua_State *L)
{
    // /t/ is borrowed (STACK).
    const GVariantType *t = fetch_gtype_borrow(L, 1, "argument #1");

    if (!g_variant_type_is_dict_entry(t)) {
        return luaL_argerror(L, 1, "is not a dict entry type");
    }

    // L: ?
    make_tobj_copy(L, g_variant_type_key(t)); // L: ? ktype
    make_tobj_copy(L, g_variant_type_value(t)); // L: ? ktype vtype
    return 2;
}

static int l_mktype_simple(lua_State *L)
{
    static const char VALID[] = {
        'b', // boolean
        'y', // byte
        'n', // int16
        'q', // uint16
        'i', // int32
        'u', // uint32
        'x', // int64
        't', // uint64
        'h', // handle
        'd', // double
        's', // string
        'o', // object_path
        'g', // signature
        'v', // variant
    };

    size_t ns;
    const char *s = luaL_checklstring(L, 1, &ns);
    if (ns != 1) {
        goto bad;
    }
    if (memchr(VALID, (unsigned char) s[0], sizeof(VALID)) == NULL) {
        goto bad;
    }
    make_tobj_steal(L, g_variant_type_new(s));
    return 1;
bad:
    return luaL_argerror(L, 1, "not a simple type name");
}

static int l_mktype_array(lua_State *L)
{
    // /t/ is borrowed (STACK).
    const GVariantType *t = fetch_gtype_borrow(L, 1, "argument #1");

    make_tobj_steal(L, g_variant_type_new_array(t));
    return 1;
}

static int l_mktype_dict_entry(lua_State *L)
{
    // Both /tk/ and /tv/ are borrowed (STACK).
    const GVariantType *tk = fetch_gtype_borrow(L, 1, "argument #1");
    const GVariantType *tv = fetch_gtype_borrow(L, 2, "argument #2");

    if (!g_variant_type_is_basic(tk)) {
        return luaL_argerror(L, 1, "key type is not a basic type");
    }

    make_tobj_steal(L, g_variant_type_new_dict_entry(tk, tv));
    return 1;
}

typedef struct {
    const GVariantType **items;
    size_t n_copied;
} MkTupleUD;

static int throwable_l_mktype_tuple(lua_State *L)
{
    MkTupleUD *ud = lua_touserdata(L, lua_upvalueindex(1));

    luaL_checktype(L, 1, LUA_TTABLE);
    size_t n = ls_lua_array_len(L, 1);

    ud->items = LS_XNEW(const GVariantType *, n);

    for (size_t i = 1; i <= n; ++i) {
        lua_rawgeti(L, 1, i);

        // /t/ is borrowed (ARRAY).
        const GVariantType *t = fetch_gtype_borrow(L, -1, "array element");
        // Since /t/ is ARRAY-borrowed, we copy it right away.
        ud->items[ud->n_copied++] = g_variant_type_copy(t);

        lua_pop(L, 1);
    }

    GVariantType *res = g_variant_type_new_tuple(ud->items, n);
    make_tobj_steal(L, res);
    return 1;
}

static int l_mktype_tuple(lua_State *L)
{
    MkTupleUD ud = {0};
    bool ok = zoo_call_prot(L, lua_gettop(L), 1, throwable_l_mktype_tuple, &ud);

    for (size_t i = 0; i < ud.n_copied; ++i) {
        g_variant_type_free((GVariantType *) ud.items[i]);
    }
    free(ud.items);

    if (ok) {
        return 1;
    } else {
        return lua_error(L);
    }
}

const GVariantType *zoo_uncvt_type_fetch_borrow(lua_State *L, int pos, const char *what)
{
    return fetch_gtype_borrow(L, pos, what);
}

void zoo_uncvt_type_bake_steal(GVariantType *t, lua_State *L)
{
    make_tobj_steal(L, t);
}

static void register_mt(lua_State *L)
{
    zoo_mt_begin(L, MT_NAME);

    zoo_mt_add_method(L, "get_type_string", l_get_type_string);
    zoo_mt_add_method(L, "get_category", l_get_category);
    zoo_mt_add_method(L, "is_basic", l_is_basic);

    zoo_mt_add_method(L, "get_item_types", l_get_item_types);
    zoo_mt_add_method(L, "get_elem_type", l_get_elem_type);
    zoo_mt_add_method(L, "get_kv_types", l_get_kv_types);

    zoo_mt_add_method(L, "equals_to", l_equals_to);

    zoo_mt_add_method(L, "__gc", tobj_gc);

    zoo_mt_end(L);
}

void zoo_uncvt_type_register_mt_and_funcs(lua_State *L)
{
    register_mt(L);

#define REG(Name_, F_) (lua_pushcfunction(L, (F_)), lua_setfield(L, -2, (Name_)))

    REG("mktype_simple", l_mktype_simple);
    REG("mktype_array", l_mktype_array);
    REG("mktype_dict_entry", l_mktype_dict_entry);
    REG("mktype_tuple", l_mktype_tuple);

#undef REG
}
