/*
 * Copyright (C) 2025  luastatus developers
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

#include "parse_opts.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include <inttypes.h>
#include <math.h>
#include <errno.h>
#include <lua.h>
#include <lauxlib.h>
#include "libls/ls_compdep.h"
#include "libls/ls_panic.h"
#include "libls/ls_assert.h"
#include "libls/ls_alloc_utils.h"
#include "libls/ls_time_utils.h"
#include "libmoonvisit/moonvisit.h"
#include "priv.h"

typedef struct {
    MoonVisit *mv;
    bool is_ok;
} Ctx;

LS_ATTR_PRINTF(2, 3)
static void set_error(Ctx *ctx, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    if (vsnprintf(ctx->mv->errbuf, ctx->mv->nerrbuf, fmt, vl) < 0) {
        LS_PANIC_WITH_ERRNUM("vsnprintf() failed", errno);
    }
    va_end(vl);

    ctx->is_ok = false;
}

static inline void do_str(Ctx *ctx, const char *key, char **dst, const char *if_omitted)
{
    if (!ctx->is_ok) {
        return;
    }
    char *res = NULL;
    if (moon_visit_str(ctx->mv, -1, key, &res, NULL, true) < 0) {
        ctx->is_ok = false;
        return;
    }
    if (!res && if_omitted) {
        res = ls_xstrdup(if_omitted);
    }
    *dst = res;
}

static inline void do_u64(Ctx *ctx, const char *key, uint64_t *dst, uint64_t max_val)
{
    if (!ctx->is_ok) {
        return;
    }
    if (moon_visit_uint(ctx->mv, -1, key, dst, true) < 0) {
        ctx->is_ok = false;
        return;
    }
    if (*dst > max_val) {
        set_error(ctx, "%s > %" PRIu64, key, max_val);
        return;
    }
}

static inline void do_bool(Ctx *ctx, const char *key, bool *dst)
{
    if (!ctx->is_ok) {
        return;
    }
    if (moon_visit_bool(ctx->mv, -1, key, dst, true) < 0) {
        ctx->is_ok = false;
        return;
    }
}

static inline void do_double(Ctx *ctx, const char *key, double *dst)
{
    if (!ctx->is_ok) {
        return;
    }
    if (moon_visit_num(ctx->mv, -1, key, dst, true) < 0) {
        ctx->is_ok = false;
        return;
    }
}

static inline void do_lfunc(Ctx *ctx, const char *key, int *dst)
{
    if (!ctx->is_ok) {
        return;
    }
    MoonVisit *mv = ctx->mv;
    // mv->L: ? table

    lua_getfield(mv->L, -1, key); // mv->L: ? table value
    if (moon_visit_checktype_at(mv, key, -1, LUA_TFUNCTION) < 0) {
        ctx->is_ok = false;
        return;
    }
    int ref = luaL_ref(mv->L, LUA_REGISTRYINDEX); // mv->L: ? table
    LS_ASSERT(ref != LUA_REFNIL);
    *dst = ref;
}

static bool req_timeout_double_to_int(double tmo, int *out)
{
    if (!isgreaterequal(tmo, 0.0)) {
        return false;
    }
    if (tmo > INT_MAX) {
        return INT_MAX;
    }
    *out = ceil(tmo);
    return true;
}

static void connection_settings_begin(Ctx *ctx)
{
    if (!ctx->is_ok) {
        return;
    }
    MoonVisit *mv = ctx->mv;

    // mv->L: ? opts
    if (moon_visit_scrutinize_table(ctx->mv, -1, "connection", true) < 0) {
        ctx->is_ok = false;
        return;
    }

    mv->where = "connection";

    if (lua_isnil(mv->L, -1)) {
        // mv->L: ? opts nil
        lua_pop(mv->L, 1); // mv->L: ? opts
        lua_newtable(mv->L); // mv->L: ? opts empty_table
        return;
    }
    // mv->L: ? opts connection
}

static inline void connection_settings_end(Ctx *ctx)
{
    if (!ctx->is_ok) {
        return;
    }
    MoonVisit *mv = ctx->mv;

    // mv->L: ? opts connection
    lua_pop(mv->L, 1); // mv->L: opts

    mv->where = NULL;
}

int parse_opts(Priv *p, MoonVisit *mv)
{
    Ctx ctx = {.mv = mv, .is_ok = true};

    uint64_t port = p->cnx_settings.port;
    uint64_t max_response_bytes = -1;
    double req_timeout = p->cnx_settings.req_timeout;

    connection_settings_begin(&ctx);

    do_str(&ctx, "hostname", &p->cnx_settings.hostname, "127.0.0.1");
    do_u64(&ctx, "port", &port, 65535);
    do_str(&ctx, "custom_iface", &p->cnx_settings.custom_iface, NULL);
    do_bool(&ctx, "use_ssl", &p->cnx_settings.use_ssl);
    do_bool(&ctx, "log_all_traffic", &p->cnx_settings.log_all_traffic);
    do_bool(&ctx, "log_response_on_error", &p->cnx_settings.log_response_on_error);
    do_u64(&ctx, "max_response_bytes", &max_response_bytes, -1);
    do_double(&ctx, "req_timeout", &req_timeout);

    connection_settings_end(&ctx);

    // This one is placed in 'p->cnx_settings', but not actually in
    // the "connection" table (instead, it is expected to be located
    // in the "outer" table).
    do_bool(&ctx, "cache_prompt", &p->cnx_settings.cache_prompt);

    do_bool(&ctx, "greet", &p->greet);
    do_double(&ctx, "upd_timeout", &p->upd_timeout);
    do_bool(&ctx, "tell_about_timeout", &p->tell_about_timeout);
    do_bool(&ctx, "report_mu", &p->report_mu);
    do_str(&ctx, "extra_params_json", &p->extra_params_json, NULL);

    do_lfunc(&ctx, "prompt", &p->lref_prompt_func);

    if (!ctx.is_ok) {
        goto error;
    }

    p->cnx_settings.port = port;

    if (max_response_bytes > UINT32_MAX) {
        max_response_bytes = UINT32_MAX;
    }
    p->cnx_settings.max_response_bytes = max_response_bytes;

    if (!req_timeout_double_to_int(req_timeout, &p->cnx_settings.req_timeout)) {
        set_error(&ctx, "invalid req_timeout");
        goto error;
    }

    if (!ls_double_is_good_time_delta(p->upd_timeout)) {
        set_error(&ctx, "invalid upd_timeout");
        goto error;
    }

    return 0;

error:
    return -1;
}
