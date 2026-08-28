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

#include "zoo.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <lua.h>

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include "libls/ls_alloc_utils.h"
#include "libls/ls_panic.h"
#include "libls/ls_lua_madness.h"

#include "zoo_call_params.h"
#include "zoo_uncvt_type.h"
#include "zoo_uncvt_val.h"
#include "zoo_registry.h"
#include "../cvt.h"
#include "../bustype2idx.h"

static void failure(lua_State *L, GError *err)
{
    lua_pushboolean(L, 0);
    ls_lua_madness_pushstr(L, err->message);
}

static void success(lua_State *L, GVariant *res)
{
    lua_pushboolean(L, 1);
    cvt(L, res);
}

static const char *lookupf(Zoo_CallParams *p, const char *key)
{
    for (Zoo_StrField *f = p->str_fields; f->key; ++f) {
        if (strcmp(key, f->key) == 0) {
            return f->value;
        }
    }
    LS_PANIC("lookupf: cannot find string field");
}

struct Zoo {
    GDBusConnection *conns[2];
};

static int do_the_bloody_thing(
    lua_State *L,
    Zoo *z,
    Zoo_CallParams *p,
    const char *dest,
    const char *object_path,
    const char *interface_name,
    const char *method_name,
    GVariant *args)
{
    // We "assume ownership" of /args/; now we need to unref it exactly once before exit.
    args = g_variant_ref_sink(args);

    GDBusConnection *conn = z->conns[bustype2idx(p->bus_type)];
    if (!conn) {
        GError *err = NULL;
        conn = g_bus_get_sync(p->bus_type, NULL, &err);
        if (!conn) {
            LS_ASSERT(err != NULL);
            failure(L, err);
            g_error_free(err);

            // Dispose of /args/.
            g_variant_unref(args);

            return 2;
        }
        LS_ASSERT(err == NULL);
        z->conns[bustype2idx(p->bus_type)] = conn;
    }

    GError *err = NULL;
    GVariant *res = g_dbus_connection_call_sync(
        conn,
        /*bus_name=*/ dest,
        /*object_path=*/ object_path,
        /*interface_name=*/ interface_name,
        /*method_name=*/ method_name,
        /*parameters=*/ args,
        /*reply_type=*/ NULL,
        /*flags=*/ p->flags,
        /*timeout_ms=*/ p->tmo_ms,
        /*cancellable=*/ NULL,
        /*error=*/ &err
    );
    // Dispose of /args/.
    g_variant_unref(args);

    if (res) {
        LS_ASSERT(err == NULL);
        success(L, res);
        g_variant_unref(res);
    } else {
        LS_ASSERT(err != NULL);
        failure(L, err);
        g_error_free(err);
    }
    return 2;
}

static int l_call_method(lua_State *L) /*__THROWABLE__*/
{
    Zoo *z = lua_touserdata(L, lua_upvalueindex(1));

    Zoo_StrField str_fields[] = {
        {.key = "dest"},
        {.key = "object_path"},
        {.key = "interface"},
        {.key = "method"},
        {0},
    };
    Zoo_CallParams p = {
        .str_fields = str_fields,
        .gvalue_field_name = "args",
        .gvalue_field_must_be_tuple = true,
    };
    zoo_call_params_parse(L, &p, 1);

    int nret = do_the_bloody_thing(
        L, z, &p,
        /*dest=*/ lookupf(&p, "dest"),
        /*object_path=*/ lookupf(&p, "object_path"),
        /*interface_name=*/ lookupf(&p, "interface"),
        /*method_name=*/ lookupf(&p, "method"),
        /*args=*/ p.gvalue
    );
    zoo_call_params_free(&p);
    return nret;
}

static int l_call_method_str(lua_State *L) /*__THROWABLE__*/
{
    Zoo *z = lua_touserdata(L, lua_upvalueindex(1));

    Zoo_StrField str_fields[] = {
        {.key = "dest"},
        {.key = "object_path"},
        {.key = "interface"},
        {.key = "method"},
        {.key = "arg_str", .nullable = true},
        {0},
    };
    Zoo_CallParams p = {
        .str_fields = str_fields,
    };
    zoo_call_params_parse(L, &p, 1);

    const char *arg_str = lookupf(&p, "arg_str");
    GVariant *args = arg_str ?
        g_variant_new("(s)", arg_str) :
        g_variant_new("()") ;

    int nret = do_the_bloody_thing(
        L, z, &p,
        /*dest=*/ lookupf(&p, "dest"),
        /*object_path=*/ lookupf(&p, "object_path"),
        /*interface_name=*/ lookupf(&p, "interface"),
        /*method_name=*/ lookupf(&p, "method"),
        /*args=*/ args
    );
    zoo_call_params_free(&p);
    return nret;
}

static int l_get_property(lua_State *L) /*__THROWABLE__*/
{
    Zoo *z = lua_touserdata(L, lua_upvalueindex(1));

    Zoo_StrField str_fields[] = {
        {.key = "dest"},
        {.key = "object_path"},
        {.key = "interface"},
        {.key = "property_name"},
        {0},
    };
    Zoo_CallParams p = {
        .str_fields = str_fields,
    };
    zoo_call_params_parse(L, &p, 1);

    int nret = do_the_bloody_thing(
        L, z, &p,
        /*dest=*/ lookupf(&p, "dest"),
        /*object_path=*/ lookupf(&p, "object_path"),
        /*interface_name=*/ "org.freedesktop.DBus.Properties",
        /*method_name=*/ "Get",
        /*args=*/ g_variant_new(
            "(ss)",
            lookupf(&p, "interface"),
            lookupf(&p, "property_name")
        )
    );
    zoo_call_params_free(&p);
    return nret;
}

static int l_get_all_properties(lua_State *L) /*__THROWABLE__*/
{
    Zoo *z = lua_touserdata(L, lua_upvalueindex(1));

    Zoo_StrField str_fields[] = {
        {.key = "dest"},
        {.key = "object_path"},
        {.key = "interface"},
        {0},
    };
    Zoo_CallParams p = {
        .str_fields = str_fields,
    };
    zoo_call_params_parse(L, &p, 1);

    int nret = do_the_bloody_thing(
        L, z, &p,
        /*dest=*/ lookupf(&p, "dest"),
        /*object_path=*/ lookupf(&p, "object_path"),
        /*interface_name=*/ "org.freedesktop.DBus.Properties",
        /*method_name=*/ "GetAll",
        /*args=*/ g_variant_new(
            "(s)",
            lookupf(&p, "interface")
        )
    );
    zoo_call_params_free(&p);
    return nret;
}

static int l_set_property_str(lua_State *L) /*__THROWABLE__*/
{
    Zoo *z = lua_touserdata(L, lua_upvalueindex(1));

    Zoo_StrField str_fields[] = {
        {.key = "dest"},
        {.key = "object_path"},
        {.key = "interface"},
        {.key = "property_name"},
        {.key = "value_str"},
        {0},
    };
    Zoo_CallParams p = {
        .str_fields = str_fields,
    };
    zoo_call_params_parse(L, &p, 1);

    int nret = do_the_bloody_thing(
        L, z, &p,
        /*dest=*/ lookupf(&p, "dest"),
        /*object_path=*/ lookupf(&p, "object_path"),
        /*interface_name=*/ "org.freedesktop.DBus.Properties",
        /*method_name=*/ "Set",
        /*args=*/ g_variant_new(
            "(ssv)",
            lookupf(&p, "interface"),
            lookupf(&p, "property_name"),
            g_variant_new("s", lookupf(&p, "value_str"))
        )
    );
    zoo_call_params_free(&p);
    return nret;
}

static int l_set_property(lua_State *L) /*__THROWABLE__*/
{
    Zoo *z = lua_touserdata(L, lua_upvalueindex(1));

    Zoo_StrField str_fields[] = {
        {.key = "dest"},
        {.key = "object_path"},
        {.key = "interface"},
        {.key = "property_name"},
        {0},
    };
    Zoo_CallParams p = {
        .str_fields = str_fields,
        .gvalue_field_name = "value",
    };
    zoo_call_params_parse(L, &p, 1);

    int nret = do_the_bloody_thing(
        L, z, &p,
        /*dest=*/ lookupf(&p, "dest"),
        /*object_path=*/ lookupf(&p, "object_path"),
        /*interface_name=*/ "org.freedesktop.DBus.Properties",
        /*method_name=*/ "Set",
        /*args=*/ g_variant_new(
            "(ssv)",
            lookupf(&p, "interface"),
            lookupf(&p, "property_name"),
            p.gvalue
        )
    );
    zoo_call_params_free(&p);
    return nret;
}

Zoo *zoo_new(void)
{
    Zoo *z = LS_XNEW(Zoo, 1);
    *z = (Zoo) {0};
    return z;
}

void zoo_register_funcs(Zoo *z, lua_State *L)
{
    // L: ? table
    lua_newtable(L); // L: ? table table
    zoo_uncvt_type_register_mt_and_funcs(L); // L: ? table table
    zoo_uncvt_val_register_mt_and_funcs(L); // L: ? table table
    lua_setfield(L, -2, "dbustypes_lowlevel"); // L: ? table

    // See 'DOCS/c_notes/lua_cfuncs.md' in the root of the repo for what __OK__ means.

    static const Zoo_RegistryEntry registry[] = {
        /*__OK__*/ ZOO_REG_ENT("call_method",        l_call_method),
        /*__OK__*/ ZOO_REG_ENT("call_method_str",    l_call_method_str),
        /*__OK__*/ ZOO_REG_ENT("get_property",       l_get_property),
        /*__OK__*/ ZOO_REG_ENT("get_all_properties", l_get_all_properties),
        /*__OK__*/ ZOO_REG_ENT("set_property",       l_set_property),
        /*__OK__*/ ZOO_REG_ENT("set_property_str",   l_set_property_str),
        {0},
    };
    lua_pushlightuserdata(L, z); // L: ? table upvalue
    zoo_registry_register_with_upvalue(L, registry); // L: ? table upvalue
    lua_pop(L, 1); // L: ? table
}

void zoo_destroy(Zoo *z)
{
    for (size_t i = 0; i < 2; ++i) {
        GDBusConnection *conn = z->conns[i];
        if (conn) {
            g_object_unref(conn);
        }
    }
    free(z);
}
