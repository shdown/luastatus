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

#include "prop_ctx_parse.h"

#include <stdbool.h>

#include <gio/gio.h>
#include <lua.h>
#include <lauxlib.h>

#include "libls/ls_alloc_utils.h"
#include "libls/ls_time_utils.h"

#include "prop_ctx.h"

static bool load_field(PCtx *ctx, const char *key, int expected_type, bool nil_is_ok)
{
    if (pctx_has_error(ctx)) {
        return false;
    }
    // ctx->L: ? table
    lua_getfield(ctx->L, -1, key); // ctx->L: ? table value
    if (nil_is_ok && lua_isnil(ctx->L, -1)) {
        lua_pop(ctx->L, 1); // ctx->
        return false;
    }
    if (lua_type(ctx->L, -1) != expected_type) {
        pctx_set_error_fatal(
            ctx, "parameter '%s': expected %s, found %s",
            key,
            lua_typename(ctx->L, expected_type),
            luaL_typename(ctx->L, -1)
        );
        lua_pop(ctx->L, 1);
        return false;
    }
    return true;
}

static void do_str(PCtx *ctx, char **dst, const char *key)
{
    // ctx->L: ? table
    if (load_field(ctx, key, LUA_TSTRING, false)) {
        // ctx->L: ? table str
        *dst = ls_xstrdup(lua_tostring(ctx->L, -1));
        lua_pop(ctx->L, 1); // ctx->L: ? table
    }
}

static void maybe_do_bool(PCtx *ctx, bool *dst, const char *key)
{
    // ctx->L: ? table
    if (load_field(ctx, key, LUA_TSTRING, true)) {
        // ctx->L: ? table bool
        *dst = lua_toboolean(ctx->L, -1);
        lua_pop(ctx->L, 1); // ctx->L: ? table
    }
}

static void maybe_do_timeout(PCtx *ctx, int *dst, const char *key)
{
    // ctx->L: ? table
    if (load_field(ctx, key, LUA_TNUMBER, true)) {
        // ctx->L: ? table number
        double d = lua_tonumber(ctx->L, -1);

        LS_TimeDelta TD;
        if (ls_double_to_TD_checked(d, &TD)) {
            *dst = ls_TD_to_poll_ms_tmo(TD);
        } else {
            pctx_set_error_fatal(
                ctx, "parameter '%s': invalid timeout",
                key
            );
        }

        lua_pop(ctx->L, 1); // ctx->L: ? table
    }
}

void pctx_parse(PCtx *ctx, bool with_property_name)
{
    do_str(ctx, &ctx->which_bus, "bus");
    do_str(ctx, &ctx->dest, "dest");
    do_str(ctx, &ctx->object_path, "object_path");
    do_str(ctx, &ctx->interface, "interface");
    if (with_property_name) {
        do_str(ctx, &ctx->property_name, "property_name");
    }

    bool no_autostart = false;
    maybe_do_bool(ctx, &no_autostart, "flag_no_autostart");
    ctx->flags = no_autostart ? G_DBUS_CALL_FLAGS_NO_AUTO_START : 0;

    ctx->tmo_ms = -1;
    maybe_do_timeout(ctx, &ctx->tmo_ms, "timeout");
}
