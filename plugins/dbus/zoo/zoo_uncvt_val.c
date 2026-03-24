#include "zoo_uncvt_val.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <glib.h>

#include "libls/ls_panic.h"
#include "libls/ls_alloc_utils.h"
#include "libls/ls_lua_compat.h"

#include "zoo_checkudata.h"
#include "zoo_call_prot.h"
#include "zoo_uncvt_type.h"
#include "zoo_mt.h"
#include "../cvt.h"

typedef struct {
    // This is an owning handle (separate reference to GVariant), not a borrow.
    // This is never a "floating reference".
    // May be NULL, which means it's already been "finalized" (garbage-collected).
    GVariant *v;
} Vobj;

static const char *MT_NAME = "io.shdown.luastatus.plugin.dbus.official.Value";

static Vobj *fetch_vobj(lua_State *L, int pos, const char *what)
{
    return zoo_checkudata(L, pos, MT_NAME, what);
}

static GVariant *fetch_gvar_borrow(lua_State *L, int pos, const char *what)
{
    Vobj *vobj = fetch_vobj(L, pos, what);
    if (!vobj->v) {
        (void) luaL_error(L, "this value object has already been finalized");
        LS_MUST_BE_UNREACHABLE();
    }
    return vobj->v;
}

static void make_vobj_steal(lua_State *L, GVariant *v)
{
    LS_ASSERT(v != NULL);
    LS_ASSERT(!g_variant_is_floating(v));

    // L: ?
    Vobj *vobj = lua_newuserdata(L, sizeof(Vobj)); // L: ? ud
    vobj->v = v;

    luaL_getmetatable(L, MT_NAME); // L: ? ud mt
    lua_setmetatable(L, -2); // L: ? ud
}

static void make_vobj_from_floating(lua_State *L, GVariant *v)
{
    LS_ASSERT(v != NULL);
    LS_ASSERT(g_variant_is_floating(v));

    make_vobj_steal(L, g_variant_ref_sink(v));
}

static int vobj_gc(lua_State *L)
{
    Vobj *vobj = fetch_vobj(L, 1, "argument #1");
    if (vobj->v) {
        g_variant_unref(vobj->v);
        vobj->v = NULL;
    }
    return 0;
}

static bool fetch_bool(lua_State *L, int arg)
{
    luaL_checktype(L, arg, LUA_TBOOLEAN);
    return lua_toboolean(L, arg);
}

static uint8_t fetch_byte(lua_State *L, int arg)
{
    int t = lua_type(L, arg);
    if (t == LUA_TNUMBER) {
        return lua_tointeger(L, arg);

    } else if (t == LUA_TSTRING) {
        size_t ns;
        const char *s = lua_tolstring(L, arg, &ns);
        if (ns != 1) {
            return luaL_argerror(L, arg, "string for 'byte' value is not of length 1");
        }
        return s[0];

    } else {
        return luaL_argerror(L, arg, "expected either a number of a string");
    }
}

static uint64_t fetch_int(lua_State *L, int arg, bool is_signed)
{
    int t = lua_type(L, arg);
    if (t == LUA_TNUMBER) {
        return lua_tointeger(L, arg);

    } else if (t == LUA_TSTRING) {
        const char *s = lua_tostring(L, arg);
        char *endptr;
        unsigned long long res;
        errno = 0;
        if (is_signed) {
            res = strtoll(s, &endptr, 0);
        } else {
            res = strtoull(s, &endptr, 0);
        }
        if (errno != 0 || s[0] == '\0' || *endptr != '\0') {
            const char *errmsg = is_signed ?
                "cannot convert this string to signed integer" :
                "cannot convert this string to unsigned integer";
            return luaL_argerror(L, arg, errmsg);
        }
        return res;

    } else {
        return luaL_argerror(L, arg, "expected either a number of a string");
    }
}

static const char *fetch_utf8(lua_State *L, int arg)
{
    size_t ns;
    const char *s = luaL_checklstring(L, arg, &ns);

    if (!g_utf8_validate_len(s, ns, NULL)) {
        (void) luaL_argerror(L, arg, "this strings contains invalid UTF-8");
        LS_MUST_BE_UNREACHABLE();
    }

    return s;
}

static GVariant *mkval_simple_impl(lua_State *L)
{
    // /t/ is borrowed (STACK).
    const GVariantType *t = zoo_uncvt_type_fetch_borrow(L, 1, "argument #1");

    if (g_variant_type_get_string_length(t) != 1) {
        goto bad_type;
    }
    const char *type_str = g_variant_type_peek_string(t);
    char type_sigil = type_str[0];

    switch (type_sigil) {

    case 'b': return g_variant_new_boolean(fetch_bool(L, 2));
    case 'y': return g_variant_new_byte(fetch_byte(L, 2));

    case 'n': return g_variant_new_int16(fetch_int(L, 2, true));
    case 'q': return g_variant_new_uint16(fetch_int(L, 2, false));

    case 'i': return g_variant_new_int32(fetch_int(L, 2, true));
    case 'u': return g_variant_new_uint32(fetch_int(L, 2, false));

    case 'x': return g_variant_new_int64(fetch_int(L, 2, true));
    case 't': return g_variant_new_uint64(fetch_int(L, 2, false));

    case 'd': return g_variant_new_double(luaL_checknumber(L, 2));

    case 's': return g_variant_new_string(fetch_utf8(L, 2));

    case 'v': return g_variant_new_variant(fetch_gvar_borrow(L, 2, "argument #2"));
    }

    if (type_sigil == 'o') {
        const char *s = luaL_checkstring(L, 2);
        if (!g_variant_is_object_path(s)) {
            (void) luaL_argerror(L, 2, "not a valid D-Bus object path");
            LS_MUST_BE_UNREACHABLE();
        }
        return g_variant_new_object_path(s);
    }

    if (type_sigil == 'g') {
        const char *s = luaL_checkstring(L, 2);
        if (!g_variant_is_signature(s)) {
            (void) luaL_argerror(L, 2, "not a valid D-Bus type signature");
            LS_MUST_BE_UNREACHABLE();
        }
        return g_variant_new_signature(s);
    }

    if (type_sigil == 'h') {
        (void) luaL_error(L, "creation of handles is not supported");
        LS_MUST_BE_UNREACHABLE();
    }

bad_type:
    (void) luaL_error(L, "not a simple type");
    LS_MUST_BE_UNREACHABLE();
}

static int l_mkval_simple(lua_State *L)
{
    make_vobj_from_floating(L, mkval_simple_impl(L));
    return 1;
}

static int l_mkval_dict_entry(lua_State *L)
{
    // Both /k/ and /v/ are borrowed (STACK).
    GVariant *k = fetch_gvar_borrow(L, 1, "argument #1");
    GVariant *v = fetch_gvar_borrow(L, 2, "argument #2");

    if (!g_variant_type_is_basic(g_variant_get_type(k))) {
        return luaL_argerror(L, 1, "key is not of a basic type");
    }

    make_vobj_from_floating(L, g_variant_new_dict_entry(k, v));
    return 1;
}

typedef struct {
    GVariant **items;
    size_t n_refd;
} MkSomethingUD;

static void free_ud(MkSomethingUD ud)
{
    for (size_t i = 0; i < ud.n_refd; ++i) {
        g_variant_unref(ud.items[i]);
    }
    free(ud.items);
}

static int throwable_l_mkval_tuple(lua_State *L)
{
    MkSomethingUD *ud = lua_touserdata(L, lua_upvalueindex(1));

    luaL_checktype(L, 1, LUA_TTABLE);
    size_t n = ls_lua_array_len(L, 1);

    ud->items = LS_XNEW(GVariant *, n);

    for (size_t i = 1; i <= n; ++i) {
        lua_rawgeti(L, 1, i);

        // /v/ is borrowed (ARRAY).
        GVariant *v = fetch_gvar_borrow(L, -1, "array element");
        // Since /v/ is ARRAY-borrowed, we ref it right away.
        ud->items[ud->n_refd++] = g_variant_ref(v);

        lua_pop(L, 1);
    }

    GVariant *res = g_variant_new_tuple(ud->items, n);
    make_vobj_from_floating(L, res);
    return 1;
}

static int l_mkval_tuple(lua_State *L)
{
    MkSomethingUD ud = {0};
    bool ok = zoo_call_prot(L, lua_gettop(L), 1, throwable_l_mkval_tuple, &ud);

    free_ud(ud);

    if (ok) {
        return 1;
    } else {
        return lua_error(L);
    }
}

static int throwable_l_mkval_array(lua_State *L)
{
    MkSomethingUD *ud = lua_touserdata(L, lua_upvalueindex(1));

    // /t/ is borrowed (STACK).
    const GVariantType *t = zoo_uncvt_type_fetch_borrow(L, 1, "argument #1");

    luaL_checktype(L, 2, LUA_TTABLE);
    size_t n = ls_lua_array_len(L, 2);

    ud->items = LS_XNEW(GVariant *, n);

    for (size_t i = 1; i <= n; ++i) {
        lua_rawgeti(L, 2, i);

        // /v/ is borrowed (ARRAY).
        GVariant *v = fetch_gvar_borrow(L, -1, "array element");

        // Check the type of /v/.
        if (!g_variant_type_equal(t, g_variant_get_type(v))) {
            char msg[128];
            snprintf(
                msg, sizeof(msg),
                "type of array element #%zu doesn't match the passed element type",
                i);
            return luaL_error(L, "%s", msg);
        }

        // Since /v/ is ARRAY-borrowed, we ref it right away.
        ud->items[ud->n_refd++] = g_variant_ref(v);

        lua_pop(L, 1);
    }

    GVariant *res = g_variant_new_array(t, ud->items, n);
    make_vobj_from_floating(L, res);
    return 1;
}

static int l_mkval_array(lua_State *L)
{
    MkSomethingUD ud = {0};
    bool ok = zoo_call_prot(L, lua_gettop(L), 1, throwable_l_mkval_array, &ud);

    free_ud(ud);

    if (ok) {
        return 1;
    } else {
        return lua_error(L);
    }
}

static int l_get_type(lua_State *L)
{
    // /v/ is borrowed (STACK).
    GVariant *v = fetch_gvar_borrow(L, 1, "argument #1");

    GVariantType *t = g_variant_type_copy(g_variant_get_type(v));

    zoo_uncvt_type_bake_steal(t, L);
    return 1;
}

static int l_equals_to(lua_State *L)
{
    // Both /a/ and /b/ are borrowed (STACK).
    GVariant *a = fetch_gvar_borrow(L, 1, "argument #1");
    GVariant *b = fetch_gvar_borrow(L, 2, "argument #2");

    lua_pushboolean(L, !!g_variant_equal(a, b));

    return 1;
}

static int l_to_lua(lua_State *L)
{
    // /v/ is borrowed (STACK).
    GVariant *v = fetch_gvar_borrow(L, 1, "argument #1");

    cvt(L, v);
    return 1;
}

GVariant *zoo_uncvt_val_fetch_newref(lua_State *L, int pos, const char *what)
{
    GVariant *v = fetch_gvar_borrow(L, pos, what);
    return g_variant_ref(v);
}

static void register_mt(lua_State *L)
{
    zoo_mt_begin(L, MT_NAME);

    zoo_mt_add_method(L, "get_type", l_get_type);
    zoo_mt_add_method(L, "equals_to", l_equals_to);
    zoo_mt_add_method(L, "to_lua", l_to_lua);

    zoo_mt_add_method(L, "__gc", vobj_gc);

    zoo_mt_end(L);
}

void zoo_uncvt_val_register_mt_and_funcs(lua_State *L)
{
    register_mt(L);

#define REG(Name_, F_) (lua_pushcfunction(L, (F_)), lua_setfield(L, -2, (Name_)))

    REG("mkval_simple", l_mkval_simple);
    REG("mkval_dict_entry", l_mkval_dict_entry);
    REG("mkval_tuple", l_mkval_tuple);
    REG("mkval_array", l_mkval_array);

#undef REG
}
