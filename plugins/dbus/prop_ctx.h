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

#ifndef prop_ctx_h_
#define prop_ctx_h_

#include <stdbool.h>
#include <lua.h>
#include <glib.h>
#include <gio/gio.h>

#include "libls/ls_compdep.h"

typedef struct {
    char *which_bus;
    char *object_path;
    char *prop_owner;
    char *prop_name;
    GDBusCallFlags flags;
    int tmo_ms;

    lua_State *L;

    bool error_fatal;
    int error_code;
    char *error_msg;
} PCtx;

PCtx pctx_new(lua_State *L);

LS_ATTR_PRINTF(2, 3)
void pctx_set_error_fatal(PCtx *ctx, const char *fmt, ...);

void pctx_set_error_gdbus(PCtx *ctx, GError *err);

bool pctx_has_error(PCtx *ctx);

int pctx_realize_error_and_destroy(PCtx *ctx);

void pctx_destroy(PCtx *ctx);

#endif
