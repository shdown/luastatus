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

#include "prop_ctx.h"
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <lua.h>

#include <glib.h>

#include "libls/ls_alloc_utils.h"
#include "libls/ls_xallocf.h"
#include "libls/ls_assert.h"

#include "prop_field.h"

PCtx pctx_new(lua_State *L, PField *fields)
{
    return (PCtx) {.L = L, .fields = fields};
}

void pctx_set_error_fatal(PCtx *ctx, const char *fmt, ...)
{
    LS_ASSERT(!pctx_has_error(ctx));

    va_list vl;
    va_start(vl, fmt);
    char *msg = ls_xallocvf(fmt, vl);
    va_end(vl);

    ctx->error_fatal = true;
    ctx->error_msg = msg;
}

void pctx_set_error_gdbus(PCtx *ctx, GError *err)
{
    LS_ASSERT(err != NULL);
    LS_ASSERT(!pctx_has_error(ctx));

    ctx->error_fatal = false;
    ctx->error_code = err->code;
    ctx->error_msg = ls_xstrdup(err->message ? err->message : "");
}

bool pctx_has_error(PCtx *ctx)
{
    return ctx->error_msg != NULL;
}

int pctx_realize_error_and_destroy(PCtx *ctx)
{
    LS_ASSERT(pctx_has_error(ctx));

    lua_State *L = ctx->L;

    if (ctx->error_fatal) {
        lua_pushstring(L, ctx->error_msg);
        pctx_destroy(ctx);
        return lua_error(L);
    } else {
        lua_pushboolean(L, 0);
        lua_pushstring(L, ctx->error_msg);
        lua_pushinteger(L, ctx->error_code);
        pctx_destroy(ctx);
        return 3;
    }
}

void pctx_destroy(PCtx *ctx)
{
    for (PField *f = ctx->fields; f->key; ++f) {
        free(f->value);
    }
    free(ctx->which_bus);
    //free(ctx->dest);
    //free(ctx->object_path);
    //free(ctx->interface);
    //free(ctx->property_name);
    free(ctx->error_msg);
}
