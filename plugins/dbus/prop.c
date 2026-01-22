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

#include "prop.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include "libls/ls_alloc_utils.h"
#include "libls/ls_assert.h"

#include "marshal.h"
#include "prop_ctx.h"
#include "prop_ctx_parse.h"

static int do_perform_call(
    GDBusConnection *conn,
    PCtx *ctx,
    const char *method_name,
    GVariant *params)
{
    LS_ASSERT(!pctx_has_error(ctx));

    GError *err = NULL;
    GVariant *res = g_dbus_connection_call_sync(
        conn,
        /*bus_name=*/ ctx->dest,
        /*object_path=*/ ctx->object_path,
        /*interface_name=*/ "org.freedesktop.DBus.Properties",
        /*method_name=*/ method_name,
        /*parameters=*/ params,
        /*reply_type=*/ NULL,
        /*flags=*/ ctx->flags,
        /*timeout_ms=*/ ctx->tmo_ms,
        /*cancellable=*/ NULL,
        /*error=*/ &err
    );
    if (res) {
        LS_ASSERT(err == NULL);
        lua_pushboolean(ctx->L, 1);
        marshal(ctx->L, res);
        g_variant_unref(res);
        return 2;
    } else {
        LS_ASSERT(err != NULL);
        pctx_set_error_gdbus(ctx, err);
        g_error_free(err);
        return -1;
    }
}

static int do_get_prop(
    GDBusConnection *conn,
    PCtx *ctx)
{
    if (pctx_has_error(ctx)) {
        return -1;
    }
    return do_perform_call(
        conn,
        ctx,
        /*method_name=*/ "Get",
        /*params=*/ g_variant_new("(ss)", ctx->interface, ctx->property_name)
    );
}

static int do_get_all_props(
    GDBusConnection *conn,
    PCtx *ctx)
{
    if (pctx_has_error(ctx)) {
        return -1;
    }
    return do_perform_call(
        conn,
        ctx,
        /*method_name=*/ "GetAll",
        /*params=*/ g_variant_new("(s)", ctx->interface)
    );
}

struct PUserdata {
    GDBusConnection *conns[2];
};

static GDBusConnection *get_conn_by_type(PCtx *ctx, GBusType type)
{
    LS_ASSERT(!pctx_has_error(ctx));

    PUserdata *ud = lua_touserdata(ctx->L, lua_upvalueindex(1));

    size_t idx = (type == G_BUS_TYPE_SYSTEM) ? 0 : 1;
    if (ud->conns[idx]) {
        return ud->conns[idx];
    }
    GError *err = NULL;
    GDBusConnection *conn = g_bus_get_sync(type, NULL, &err);
    if (!conn) {
        LS_ASSERT(err != NULL);
        pctx_set_error_gdbus(ctx, err);
        g_error_free(err);
        return NULL;
    }
    LS_ASSERT(err == NULL);
    ud->conns[idx] = conn;
    return conn;
}

static GDBusConnection *get_conn(PCtx *ctx)
{
    if (pctx_has_error(ctx)) {
        return NULL;
    }

    if (strcmp(ctx->which_bus, "system") == 0) {
        return get_conn_by_type(ctx, G_BUS_TYPE_SYSTEM);

    } else if (strcmp(ctx->which_bus, "session") == 0) {
        return get_conn_by_type(ctx, G_BUS_TYPE_SESSION);

    } else {
        pctx_set_error_fatal(
            ctx, "invalid bus name '%s' (expected either 'system' or 'session')",
            ctx->which_bus
        );
        return NULL;
    }
}

static int l_get_property(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);

    PCtx ctx = pctx_new(L);
    pctx_parse(&ctx, true);

    GDBusConnection *conn = get_conn(&ctx);
    int nret = do_get_prop(conn, &ctx);

    if (pctx_has_error(&ctx)) {
        return pctx_realize_error_and_destroy(&ctx);
    } else {
        pctx_destroy(&ctx);
        return nret;
    }
}

static int l_get_all_properties(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_settop(L, 1);

    PCtx ctx = pctx_new(L);
    pctx_parse(&ctx, false);

    GDBusConnection *conn = get_conn(&ctx);
    int nret = do_get_all_props(conn, &ctx);

    if (pctx_has_error(&ctx)) {
        return pctx_realize_error_and_destroy(&ctx);
    } else {
        pctx_destroy(&ctx);
        return nret;
    }
}

PUserdata *p_new(void)
{
    PUserdata *ud = LS_XNEW(PUserdata, 1);
    *ud = (PUserdata) {0};
    return ud;
}

static inline void register_closure(
    PUserdata *ud,
    lua_State *L,
    lua_CFunction f,
    const char *key)
{
    // L: ? table
    lua_pushlightuserdata(L, ud); // L: ? table ud
    lua_pushcclosure(L, f, 1); // L: ? table func
    lua_setfield(L, -2, key); // L: ? table
}

void p_register_funcs(PUserdata *ud, lua_State *L)
{
    register_closure(ud, L, l_get_property, "get_property");
    register_closure(ud, L, l_get_all_properties, "get_all_properties");
}

void p_destroy(PUserdata *ud)
{
    for (size_t i = 0; i < 2; ++i) {
        GDBusConnection *conn = ud->conns[i];
        if (conn) {
            g_dbus_connection_close_sync(conn, NULL, NULL);
            g_object_unref(conn);
        }
    }
    free(ud);
}
