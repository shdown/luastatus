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

#include "zoo_call_params.h"
#include <stdlib.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "libls/ls_alloc_utils.h"
#include "libls/ls_panic.h"
#include "libls/ls_time_utils.h"

#include "zoo_uncvt_val.h"
#include "zoo_call_prot.h"

static void check_type(lua_State *L, int pos, const char *key, int expected_type, bool nullable)
{
    int t = lua_type(L, pos);
    if (t != expected_type) {
        (void) luaL_error(
            L, "%s: expected %s%s, found %s",
            key,
            lua_typename(L, expected_type),
            nullable ? " or nil" : "",
            lua_typename(L, t));
        LS_MUST_BE_UNREACHABLE();
    }
}

static void do_str(lua_State *L, const char *key, char **dst, bool nullable)
{
    // L: ? table
    lua_getfield(L, 1, key); // L: ? table value
    if (nullable && lua_isnil(L, -1)) {
        goto done;
    }

    check_type(L, -1, key, LUA_TSTRING, nullable);

    size_t ns;
    const char *s = lua_tolstring(L, -1, &ns);
    if (!g_utf8_validate_len(s, ns, NULL)) {
        (void) luaL_error(L, "%s: value contains invalid UTF-8", key);
        LS_MUST_BE_UNREACHABLE();
    }

    *dst = ls_xstrdup(s);
done:
    lua_pop(L, 1); // L: ? table
}

static void do_gvalue(lua_State *L, const char *key, GVariant **dst, bool must_be_tuple)
{
    // L: ? table
    lua_getfield(L, 1, key); // L: ? table value

    GVariant *v = zoo_uncvt_val_fetch_newref(L, -1, key);
    *dst = v;
    if (must_be_tuple) {
        if (!g_variant_type_is_tuple(g_variant_get_type(v))) {
            (void) luaL_error(L, "%s: is not of tuple type", key);
            LS_MUST_BE_UNREACHABLE();
        }
    }

    lua_pop(L, 1); // L: ? table
}

static void maybe_do_bool(lua_State *L, const char *key, bool *dst)
{
    // L: ? table
    lua_getfield(L, 1, key); // L: ? table value
    if (lua_isnil(L, -1)) {
        goto done;
    }

    check_type(L, -1, key, LUA_TBOOLEAN, true);

    *dst = lua_toboolean(L, -1);
done:
    lua_pop(L, 1); // L: ? table
}

static void maybe_do_timeout(lua_State *L, const char *key, int *dst)
{
    // L: ? table
    lua_getfield(L, 1, key); // L: ? table value
    if (lua_isnil(L, -1)) {
        goto done;
    }

    check_type(L, -1, key, LUA_TNUMBER, true);

    double d = lua_tonumber(L, -1);

    LS_TimeDelta TD;
    if (ls_double_to_TD_checked(d, &TD)) {
        *dst = ls_TD_to_poll_ms_tmo(TD);
    } else {
        (void) luaL_error(L, "%s: invalid timeout value", key);
        LS_MUST_BE_UNREACHABLE();
    }

done:
    lua_pop(L, 1); // L: ? table
}

static void do_bus_type(lua_State *L, const char *key, GBusType *dst)
{
    // L: ? table
    lua_getfield(L, 1, key); // L: ? table value

    check_type(L, -1, key, LUA_TSTRING, false);

    const char *s = lua_tostring(L, -1);
    if (strcmp(s, "system") == 0) {
        *dst = G_BUS_TYPE_SYSTEM;
    } else if (strcmp(s, "session") == 0) {
        *dst = G_BUS_TYPE_SESSION;
    } else {
        (void) luaL_error(L, "%s: expected either 'system' or 'session'", key);
        LS_MUST_BE_UNREACHABLE();
    }

    lua_pop(L, 1); // L: ? table
}

static int do_parse_throwable(lua_State *L)
{
    Zoo_CallParams *p = lua_touserdata(L, lua_upvalueindex(1));

    LS_ASSERT(lua_gettop(L) == 1);

    do_bus_type(L, "bus", &p->bus_type);

    for (Zoo_StrField *f = p->str_fields; f->key; ++f) {
        do_str(L, f->key, &f->value, f->nullable);
    }

    if (p->gvalue_field_name) {
        do_gvalue(L, p->gvalue_field_name, &p->gvalue, p->gvalue_field_must_be_tuple);
    }

    bool no_autostart = false;
    maybe_do_bool(L, "flag_no_autostart", &no_autostart);
    p->flags = no_autostart ? G_DBUS_CALL_FLAGS_NO_AUTO_START : 0;

    p->tmo_ms = -1;
    maybe_do_timeout(L, "timeout", &p->tmo_ms);

    return 0;
}

void zoo_call_params_free(Zoo_CallParams *p)
{
    for (Zoo_StrField *f = p->str_fields; f->key; ++f) {
        free(f->value);
    }
    if (p->gvalue) {
        g_variant_unref(p->gvalue);
    }
}

void zoo_call_params_parse(lua_State *L, Zoo_CallParams *p, int arg)
{
    luaL_checktype(L, arg, LUA_TTABLE);

    lua_pushvalue(L, arg);
    if (!zoo_call_prot(L, 1, 0, do_parse_throwable, p)) {
        zoo_call_params_free(p);
        lua_error(L);
    }
}
