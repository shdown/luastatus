/*
 * Copyright (C) 2015-2026  luastatus developers
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

#include <lua.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <poll.h>
#include <sys/types.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "libls/ls_alloc_utils.h"
#include "libls/ls_evloop_lfuncs.h"
#include "libls/ls_io_utils.h"
#include "libls/ls_time_utils.h"
#include "libls/ls_string.h"
#include "libls/ls_strarr.h"
#include "libls/ls_lua_compat.h"
#include "libls/ls_tls_ebuf.h"

#include "next_request_params.h"
#include "set_error.h"
#include "parse_opts.h"
#include "make_request.h"
#include "opts.h"
#include "compat_lua_resume.h"

#include "mod_json/mod_json.h"
#include "mod_urlencode/mod_urlencode.h"

typedef struct {
    int global_req_flags;

    lua_State *coro;
    int lref;

    int pipefds[2];
} Priv;

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;

    ls_close(p->pipefds[0]);
    ls_close(p->pipefds[1]);

    free(p);
}

static lua_State *make_coro(lua_State *L, int *out_lref)
{
    // L: ? func
    lua_State *coro = lua_newthread(L); // L: ? func thread
    *out_lref = luaL_ref(L, LUA_REGISTRYINDEX); // L: ? func
    lua_xmove(L, coro, 1);
    // L: ?
    // coro: func
    return coro;
}

static bool parse_planner(MoonVisit *mv, Priv *p)
{
    lua_State *L = mv->L;
    // L: ? table
    lua_getfield(L, -1, "planner"); // L: ? table planner
    if (moon_visit_checktype_at(mv, "planner", -1, LUA_TFUNCTION) < 0) {
        return false;
    }
    p->coro = make_coro(L, &p->lref); // L: ? table
    return true;
}

static int init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .global_req_flags = 0,
        .coro = NULL,
        .lref = LUA_REFNIL,
        .pipefds = {-1, -1},
    };

    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse with_headers_global
    bool with_headers_global = false;
    if (moon_visit_bool(&mv, -1, "with_headers_global", &with_headers_global, true) < 0) {
        goto mverror;
    }
    if (with_headers_global) {
        p->global_req_flags |= REQ_FLAG_NEEDS_HEADERS;
    }

    // Parse debug_global
    bool debug_global = false;
    if (moon_visit_bool(&mv, -1, "debug_global", &debug_global, true) < 0) {
        goto mverror;
    }
    if (debug_global) {
        p->global_req_flags |= REQ_FLAG_DEBUG;
    }

    // Parse make_self_pipe
    bool mkpipe = false;
    if (moon_visit_bool(&mv, -1, "make_self_pipe", &mkpipe, true) < 0) {
        goto mverror;
    }
    if (mkpipe) {
        if (ls_self_pipe_open(p->pipefds) < 0) {
            LS_FATALF(pd, "ls_self_pipe_open: %s", ls_tls_strerror(errno));
            goto error;
        }
    }

    // Parse planner
    if (!parse_planner(&mv, p)) {
        goto mverror;
    }

    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static int l_get_supported_opts(lua_State *L)
{
    lua_createtable(L, 0, OPTS_NUM); // L: ? table
    for (size_t i = 0; i < OPTS_NUM; ++i) {
        lua_pushboolean(L, 1); // L: ? table true
        lua_setfield(L, -2, OPTS[i].spelling); // L: ? table
    }
    return 1;
}

static int l_time_now(lua_State *L)
{
    double res = ls_timespec_to_raw_double(ls_now_timespec());
    lua_pushnumber(L, res);
    return 1;
}

static void register_funcs(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv;
    // L: table
    ls_self_pipe_push_luafunc(p->pipefds, L); // L: table func
    lua_setfield(L, -2, "wake_up"); // L: table

    lua_pushcfunction(L, l_get_supported_opts); // L: table func
    lua_setfield(L, -2, "get_supported_opts");

    lua_pushcfunction(L, l_time_now); // L: table func
    lua_setfield(L, -2, "time_now");

    mod_json_register_funcs(L);

    mod_urlencode_register_funcs(L);
}

typedef enum {
    NACT_REQUEST,
    NACT_SLEEP,
    NACT_CALL_CB,
    NACT__LAST,
} NextAction;

typedef enum {
    WUPSTAT_NO_WAKEUP,
    WUPSTAT_YES_WAKEUP,
    WUPSTAT_NOT_APPLICABLE,
} WakeupStatus;

typedef struct {
    NextAction nact;

    NextRequestParams next_req_params;
    LS_TimeDelta TD;
    char *what;

    WakeupStatus wakeup_status;
} Ctx;

static void clear_next_req_params(NextRequestParams *X)
{
    if (X->headers) {
        curl_slist_free_all(X->headers);
        X->headers = NULL;
    }
    curl_easy_reset(X->C);
    X->local_req_flags = 0;
}

static void destroy_ctx(Ctx *ctx)
{
    clear_next_req_params(&ctx->next_req_params);
    free(ctx->what);
    curl_easy_cleanup(ctx->next_req_params.C);
}

static bool parseY_request(lua_State *L, Ctx *ctx, char **out_errmsg)
{
    // L: ? table
    lua_getfield(L, -1, "params"); // L: ? table params
    if (!lua_istable(L, -1)) {
        set_type_error(out_errmsg, L, -1, LUA_TTABLE, "'params' field: ");
        return false;
    }
    return parse_opts(&ctx->next_req_params, L, out_errmsg);
}

static bool parseY_sleep(lua_State *L, Ctx *ctx, char **out_errmsg)
{
    // L: ? table
    lua_getfield(L, -1, "period"); // L: ? table period
    if (!lua_isnumber(L, -1)) {
        set_type_error(out_errmsg, L, -1, LUA_TNUMBER, "'period' field: ");
        return false;
    }
    double d = lua_tonumber(L, -1);
    if (!ls_double_to_TD_checked(d, &ctx->TD)) {
        set_error(out_errmsg, "invalid period");
        return false;
    }
    return true;
}

static bool parseY_call_cb(lua_State *L, Ctx *ctx, char **out_errmsg)
{
    // L: ? table
    lua_getfield(L, -1, "what"); // L: ? table what
    if (!lua_isstring(L, -1)) {
        set_type_error(out_errmsg, L, -1, LUA_TSTRING, "'what' field: ");
        return false;
    }

    free(ctx->what);
    ctx->what = ls_xstrdup(lua_tostring(L, -1));

    return true;
}

static NextAction parse_action(const char *s)
{
    if (strcmp(s, "request") == 0) {
        return NACT_REQUEST;
    }
    if (strcmp(s, "sleep") == 0) {
        return NACT_SLEEP;
    }
    if (strcmp(s, "call_cb") == 0) {
        return NACT_CALL_CB;
    }
    return NACT__LAST;
}

static bool parseY(lua_State *L, Ctx *ctx, char **out_errmsg)
{
    if (!lua_istable(L, -1)) {
        set_type_error(out_errmsg, L, -1, LUA_TTABLE, "yielded value: ");
        return false;
    }
    // L: ? table

    lua_getfield(L, -1, "action"); // L: ? table action
    if (!lua_isstring(L, -1)) {
        set_type_error(out_errmsg, L, -1, LUA_TSTRING, "'action' field: ");
        return false;
    }
    ctx->nact = parse_action(lua_tostring(L, -1));
    lua_pop(L, 1); // L: ? table

    switch (ctx->nact) {
    case NACT_REQUEST:
        clear_next_req_params(&ctx->next_req_params);
        return parseY_request(L, ctx, out_errmsg);

    case NACT_SLEEP:
        return parseY_sleep(L, ctx, out_errmsg);

    case NACT_CALL_CB:
        return parseY_call_cb(L, ctx, out_errmsg);

    case NACT__LAST:
        set_error(out_errmsg, "unknown action");
        return false;
    }
    LS_MUST_BE_UNREACHABLE();
}

static inline void push_wakeup_status(lua_State *L, WakeupStatus wakeup_status)
{
    if (wakeup_status == WUPSTAT_NOT_APPLICABLE) {
        lua_pushnil(L);
    } else {
        lua_pushboolean(L, wakeup_status == WUPSTAT_YES_WAKEUP);
    }
}

static inline void push_planner_by_lref(Priv *p, lua_State *L, lua_State *coro)
{
    // L: ?
    // coro: ?
    lua_rawgeti(L, LUA_REGISTRYINDEX, p->lref); // L: ? thread
    lua_xmove(L, coro, 1);
    // L: ?
    // coro: ? thread
}

static bool make_flash_call(
    LuastatusPluginData *pd,
    LuastatusPluginRunFuncs funcs,
    Ctx *ctx)
{
    Priv *p = pd->priv;

    lua_State *L = funcs.call_begin(pd->userdata);
    lua_State *coro = p->coro;

    int L_orig_top = lua_gettop(L);
    int coro_orig_top = lua_gettop(coro);

    char *errmsg = NULL;
    bool is_ok;

    // L: ?
    push_wakeup_status(L, ctx->wakeup_status); // L: ? wakeup_status
    ctx->wakeup_status = WUPSTAT_NOT_APPLICABLE;

    int nresults;
    push_planner_by_lref(p, L, coro); // coro: thread
    int rc = compat_lua_resume(coro, L, 1, &nresults);
    if (rc == LUA_YIELD) {
        if (nresults != 1) {
            set_error(&errmsg, "expected 1 yielded value, got %d", nresults);
            is_ok = false;
            goto done;
        }
        is_ok = parseY(coro, ctx, &errmsg);
        goto done;
    } else if (rc == 0) {
        // coro: uh, something; doesn't matter actually
        set_error(&errmsg, "coroutine finished its execution");
        is_ok = false;
        goto done;
    } else {
        // coro: err
        const char *lua_errmsg = lua_tostring(coro, -1);
        if (!lua_errmsg) {
            lua_errmsg = "(Lua object cannot be converted to string)";
        }
        set_error(&errmsg, "Lua error in planner: %s", lua_errmsg);
        is_ok = false;
        goto done;
    }

done:
    lua_settop(coro, coro_orig_top); // coro: -
    lua_settop(L, L_orig_top); // L: ?
    funcs.call_cancel(pd->userdata);

    if (!is_ok) {
        LS_ASSERT(errmsg != NULL);
        LS_FATALF(pd, "%s", errmsg);
    }

    free(errmsg);

    return is_ok;
}

static void action_call_cb(
    LuastatusPluginData *pd,
    LuastatusPluginRunFuncs funcs,
    Ctx *ctx)
{
    lua_State *L = funcs.call_begin(pd->userdata);

    // L: ?
    lua_createtable(L, 0, 1); // L: ? table
    lua_pushstring(L, ctx->what); // L: ? table str
    lua_setfield(L, -2, "what"); // L: ? table

    funcs.call_end(pd->userdata);

    free(ctx->what);
    ctx->what = NULL;
}

static void push_headers(lua_State *L, LS_StringArray headers)
{
    size_t n = ls_strarr_size(headers);

    if (n > (size_t) LS_LUA_MAXI) {
        lua_pushnil(L);
        return;
    }

    lua_createtable(L, ls_lua_num_prealloc(n), 0); // L: ? table headers

    for (size_t i = 0; i < n; ++i) {
        size_t ns;
        const char *s = ls_strarr_at(headers, i, &ns);
        lua_pushlstring(L, s, ns); // L: ? table headers str
        lua_rawseti(L, -2, i + 1); // L: ? table headers
    }
}

static void report_request_result_ok(
    LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs,
    long status,
    const char *body, size_t nbody,
    const LS_StringArray *p_headers)
{
    lua_State *L = funcs.call_begin(pd->userdata);

    // L: ?
    lua_createtable(L, 0, 3); // L: ? table

    lua_pushstring(L, "response"); // L: ? table str
    lua_setfield(L, -2, "what"); // L: ? table

    lua_pushinteger(L, status); // L: ? table status
    lua_setfield(L, -2, "status"); // L: ? table

    if (p_headers) {
        push_headers(L, *p_headers);
        lua_setfield(L, -2, "headers"); // L: ? table
    }

    lua_pushlstring(L, body, nbody); // L: ? table body
    lua_setfield(L, -2, "body"); // L: ? table

    funcs.call_end(pd->userdata);
}

static void report_request_result_error(
    LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs,
    const char *err_descr,
    bool with_headers)
{
    lua_State *L = funcs.call_begin(pd->userdata);

    // L: ?
    lua_createtable(L, 0, 4); // L: ? table

    lua_pushstring(L, "response"); // L: ? table str
    lua_setfield(L, -2, "what"); // L: ? table

    lua_pushinteger(L, 0); // L: ? table status
    lua_setfield(L, -2, "status"); // L: ? table

    lua_pushstring(L, ""); // L: ? table body
    lua_setfield(L, -2, "body"); // L: ? table

    if (with_headers) {
        lua_newtable(L); // L: ? table headers
        lua_setfield(L, -2, "headers"); // L: ? table
    }

    lua_pushstring(L, err_descr); // L: ? table body
    lua_setfield(L, -2, "error"); // L: ? table

    funcs.call_end(pd->userdata);
}

static void action_request(
    LuastatusPluginData *pd,
    LuastatusPluginRunFuncs funcs,
    Ctx *ctx)
{
    Priv *p = pd->priv;

    int req_flags = p->global_req_flags | ctx->next_req_params.local_req_flags;

    bool with_headers = req_flags & REQ_FLAG_NEEDS_HEADERS;

    Response resp;
    char *errmsg;
    if (make_request(pd, req_flags, ctx->next_req_params.C, &resp, &errmsg)) {
        report_request_result_ok(
            pd, funcs,
            resp.status,
            resp.body.data, resp.body.size,
            with_headers ? &resp.headers : NULL);
    } else {
        report_request_result_error(pd, funcs, errmsg, with_headers);
        free(errmsg);
    }

    ls_string_free(resp.body);
    ls_strarr_destroy(resp.headers);
}

static void action_sleep(LuastatusPluginData *pd, Ctx *ctx)
{
    Priv *p = pd->priv;

    if (p->pipefds[0] >= 0) {
        int rc = ls_wait_input_on_fd(p->pipefds[0], ctx->TD);
        if (rc < 0) {
            LS_PANIC_WITH_ERRNUM("ls_wait_input_on_fd() failed", errno);
        }
        if (rc) {
            char ignored;
            ssize_t num_read = read(p->pipefds[0], &ignored, 1);
            (void) num_read;

            ctx->wakeup_status = WUPSTAT_YES_WAKEUP;
        } else {
            ctx->wakeup_status = WUPSTAT_NO_WAKEUP;
        }
    } else {
        ls_sleep(ctx->TD);
    }
}

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    CURL *C = curl_easy_init();
    if (!C) {
        LS_FATALF(pd, "curl_easy_init() failed");
        return;
    }

    Ctx ctx = {
        .nact = NACT__LAST,
        .next_req_params = {
            .C = C,
            .headers = NULL,
            .local_req_flags = 0,
        },
        .TD = {0},
        .what = NULL,
        .wakeup_status = WUPSTAT_NOT_APPLICABLE,
    };

    for (;;) {
        if (!make_flash_call(pd, funcs, &ctx)) {
            break;
        }
        switch (ctx.nact) {
        case NACT_REQUEST:
            action_request(pd, funcs, &ctx);
            break;
        case NACT_SLEEP:
            action_sleep(pd, &ctx);
            break;
        case NACT_CALL_CB:
            action_call_cb(pd, funcs, &ctx);
            break;
        case NACT__LAST:
            LS_MUST_BE_UNREACHABLE();
        }
    }

    destroy_ctx(&ctx);
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .run = run,
    .destroy = destroy,
};
