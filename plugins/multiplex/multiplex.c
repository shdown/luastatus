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

#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#include <stdio.h>

#include <lua.h>
#include <lauxlib.h>

#include "libls/ls_panic.h"
#include "libls/ls_tls_ebuf.h"
#include "libls/ls_alloc_utils.h"
#include "libls/ls_string.h"

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "wspec_list.h"
#include "runner.h"
#include "conq.h"
#include "priv.h"
#include "runtime_data.h"
#include "map_ref.h"

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;

    LS_PTH_CHECK(pthread_mutex_destroy(&p->runners_mtx));

    wspec_list_destroy(&p->wspecs);

    map_ref_destroy(p->map_ref);

    free(p);
}

static int visit_wspec(MoonVisit *mv, void *ud, int kpos, int vpos)
{
    Priv *p = ud;

    if (moon_visit_checktype_at(mv, "key", kpos, LUA_TSTRING) < 0) {
        return -1;
    }
    if (moon_visit_checktype_at(mv, "value", vpos, LUA_TSTRING) < 0) {
        return -1;
    }

    const char *name = lua_tostring(mv->L, kpos);
    const char *code = lua_tostring(mv->L, vpos);

    wspec_list_add(&p->wspecs, name, code);

    return 0;
}

static bool check_wspecs(WspecList *x, char *errbuf, size_t nerrbuf)
{
    size_t n = wspec_list_size(x);
    if (!n) {
        snprintf(errbuf, nerrbuf, "data_sources table is empty");
        return false;
    }
    if (n > CONQ_MAX_SLOTS) {
        snprintf(
            errbuf, nerrbuf,
            "too many data sources (%zu, limit is %d)",
            n, (int) CONQ_MAX_SLOTS
        );
        return false;
    }

    const char *dup = wspec_list_find_duplicates(x);
    if (dup) {
        snprintf(errbuf, nerrbuf, "data_sources table: duplicate key '%s'", dup);
        return false;
    }

    return true;
}

static int init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .wspecs = wspec_list_new(),
        .greet = false,
        .runners = NULL,
        .map_ref = map_ref_new_empty(),
    };
    LS_PTH_CHECK(pthread_mutex_init(&p->runners_mtx, NULL));
    map_ref_init(&p->map_ref, pd);

    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse data_sources
    if (moon_visit_table_f(&mv, -1, "data_sources", visit_wspec, p, false) < 0) {
        goto mverror;
    }
    if (!check_wspecs(&p->wspecs, errbuf, sizeof(errbuf))) {
        goto mverror;
    }

    // Parse greet
    if (moon_visit_bool(&mv, -1, "greet", &p->greet, true) < 0) {
        goto mverror;
    }

    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
//error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static inline const char *evt_result_to_str(RunnerEventResult val)
{
    switch (val) {
    case EVT_RESULT_OK:
        return NULL;
    case EVT_RESULT_LUA_ERROR:
        return "lua-error-in-handler";
    case EVT_RESULT_NO_HANDLER:
        return "no-handler";
    }
    LS_MUST_BE_UNREACHABLE();
}

static int l_call_event(lua_State *L)
{
    Priv *p = lua_touserdata(L, lua_upvalueindex(1));

    const char *data_src_name = luaL_checkstring(L, 1);
    int data_src_idx = wspec_list_find(&p->wspecs, data_src_name);
    if (data_src_idx < 0) {
        return luaL_error(L, "cannot find data source with name '%s'", data_src_name);
    }

    size_t ndata;
    const char *data = luaL_checklstring(L, 2, &ndata);

    Runner **R;

    LS_PTH_CHECK(pthread_mutex_lock(&p->runners_mtx));
    if (p->runners) {
        R = &p->runners[data_src_idx];
    } else {
        R = NULL;
    }
    LS_PTH_CHECK(pthread_mutex_unlock(&p->runners_mtx));

    if (!R) {
        return luaL_error(L, "runners have not been spawned yet");
    }

    const char *err;
    if (*R) {
        lua_State *borrowed_L = runner_event_begin(*R);
        lua_pushlstring(borrowed_L, data, ndata);
        RunnerEventResult res = runner_event_end(*R);
        err = evt_result_to_str(res);
    } else {
        err = "widget-failed-to-init";
    }

    if (err) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, err);
        return 2;
    } else {
        lua_pushboolean(L, 1);
        return 1;
    }
}

static void register_funcs(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv;

    // L: table
    lua_pushlightuserdata(L, p); // L: table ud
    lua_pushcclosure(L, l_call_event, 1); // L: table func
    lua_setfield(L, -2, "call_event"); // L: table
}

static void *my_thread_func(void *vud)
{
    UniversalUserdata *u_ud = vud;
    LuastatusPluginData *pd = u_ud->ectx;
    Priv *p = pd->priv;
    Runner *R = p->runners[u_ud->i];

    LS_ASSERT(R != NULL);

    runner_run(R);

    return NULL;
}

static void recv_handle_new_data(
        LuastatusPluginData *pd,
        Conq *cq,
        size_t i,
        lua_State *L)
{
    if (lua_isnil(L, -1)) {
        conq_update_slot(cq, i, NULL, 0, CONQ_SLOT_STATE_NIL);

    } else if (lua_isstring(L, -1)) {
        size_t ns;
        const char *s = lua_tolstring(L, -1, &ns);
        conq_update_slot(cq, i, s, ns, CONQ_SLOT_STATE_HAS_VALUE);

    } else {
        Priv *p = pd->priv;
        const char *name = wspec_list_get_name(&p->wspecs, i);
        LS_WARNF(
            pd, "cb of widget [%s] returned %s value (expected string or nil)",
            name,
            luaL_typename(L, -1)
        );
        conq_update_slot(cq, i, NULL, 0, CONQ_SLOT_STATE_ERROR_BAD_TYPE);
    }
}

static void recv_callback(void *vud, RunnerCallbackReason reason, lua_State *L)
{
    UniversalUserdata *u_ud = vud;
    LuastatusPluginData *pd = u_ud->ectx;
    Priv *p = pd->priv;

    Conq *cq = p->rtdata.cq;
    size_t i = u_ud->i;

    switch (reason) {
    case REASON_NEW_DATA:
        recv_handle_new_data(pd, cq, i, L);
        return;
    case REASON_LUA_ERROR:
        conq_update_slot(cq, i, NULL, 0, CONQ_SLOT_STATE_ERROR_LUA_ERR);
        return;
    case REASON_PLUGIN_EXITED:
        conq_update_slot(cq, i, NULL, 0, CONQ_SLOT_STATE_ERROR_PLUGIN_DONE);
        return;
    }

    LS_MUST_BE_UNREACHABLE();
}

static void make_call_simple(
        LuastatusPluginData *pd,
        LuastatusPluginRunFuncs funcs,
        const char *what)
{
    lua_State *L = funcs.call_begin(pd->userdata);

    lua_createtable(L, 0, 1); // L: table
    lua_pushstring(L, what); // L: table string
    lua_setfield(L, -2, "what"); // L: table

    funcs.call_end(pd->userdata);
}

static void push_update(lua_State *L, const LS_String *buf, ConqSlotState state)
{
    switch (state) {
    case CONQ_SLOT_STATE_EMPTY:
        lua_pushnil(L);
        return;
    case CONQ_SLOT_STATE_NIL:
        lua_pushboolean(L, 0);
        return;
    case CONQ_SLOT_STATE_HAS_VALUE:
        lua_pushlstring(L, buf->data, buf->size);
        return;
    case CONQ_SLOT_STATE_ERROR_PLUGIN_DONE:
    case CONQ_SLOT_STATE_ERROR_LUA_ERR:
    case CONQ_SLOT_STATE_ERROR_BAD_TYPE:
        lua_pushinteger(L, (int) state);
        return;
    }
    LS_MUST_BE_UNREACHABLE();
}

static void make_call_update(
        LuastatusPluginData *pd,
        LuastatusPluginRunFuncs funcs,
        size_t n,
        CONQ_MASK mask,
        LS_String *bufs,
        ConqSlotState *states)
{
    Priv *p = pd->priv;

    lua_State *L = funcs.call_begin(pd->userdata);

    lua_createtable(L, 0, 2); // L: table

    lua_pushstring(L, "update"); // L: table string
    lua_setfield(L, -2, "what"); // L: table

    lua_newtable(L); // L: table table
    for (size_t i = 0; i < n; ++i) {
        if ((mask >> i) & 1) {
            push_update(L, &bufs[i], states[i]); // L: table table value
            const char *key = wspec_list_get_name(&p->wspecs, i);
            lua_setfield(L, -2, key); // L: table table
        }
    }

    lua_setfield(L, -2, "updates"); // L: table

    funcs.call_end(pd->userdata);
}

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    size_t n = wspec_list_size(&p->wspecs);

    runtime_data_init(&p->rtdata, pd, n);

    map_ref_lock_mtx(p->map_ref);

    for (size_t i = 0; i < n; ++i) {
        const char *name = wspec_list_get_name(&p->wspecs, i);
        const char *code = wspec_list_get_code(&p->wspecs, i);

        Runner *R = runner_new(
            name,
            code,
            pd,
            recv_callback,
            &p->rtdata.u_uds[i]
        );
        p->rtdata.runners[i] = R;

        wspec_list_get_rid_of_code(&p->wspecs, i);
    }

    map_ref_unlock_mtx(p->map_ref);

    LS_PTH_CHECK(pthread_mutex_lock(&p->runners_mtx));
    p->runners = p->rtdata.runners;
    LS_PTH_CHECK(pthread_mutex_unlock(&p->runners_mtx));

    if (p->greet) {
        make_call_simple(pd, funcs, "hello");
    }

    for (size_t i = 0; i < n; ++i) {
        if (!p->rtdata.runners[i]) {
            continue;
        }
        pthread_t ignored_tid;
        int err_num = pthread_create(
            &ignored_tid,
            NULL,
            my_thread_func,
            &p->rtdata.u_uds[i]
        );
        if (err_num != 0) {
            LS_ERRF(pd, "cannot create thread: %s", ls_tls_strerror(err_num));
        }
    }

    Conq *cq = p->rtdata.cq;
    LS_String *bufs = p->rtdata.bufs;
    ConqSlotState *states = p->rtdata.states;
    for (;;) {
        CONQ_MASK mask = conq_fetch_updates(cq, bufs, states);
        make_call_update(pd, funcs, n, mask, bufs, states);
    }
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .run = run,
    .destroy = destroy,
};
