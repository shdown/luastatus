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

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <lua.h>
#include <lauxlib.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "libls/ls_algo.h"
#include "libls/ls_string.h"
#include "libls/ls_tls_ebuf.h"
#include "libls/ls_time_utils.h"
#include "libls/ls_evloop_lfuncs.h"
#include "libls/ls_io_utils.h"
#include "libls/ls_alloc_utils.h"
#include "libls/ls_panic.h"
#include "libls/ls_assert.h"

#include "libsafe/safev.h"

#include "conc_queue.h"
#include "requester.h"
#include "my_error.h"
#include "dss_list.h"
#include "pushed_str.h"
#include "escape_q.h"
#include "escape_json_str.h"
#include "mini_luastatus.h"
#include "describe_lua_err.h"
#include "map_ref.h"
#include "priv.h"
#include "parse_opts.h"
#include "report.h"

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    priv_destroy_inner(p);
    free(p);
}

static bool init_requester(LuastatusPluginData *pd)
{
    MyError e = {0};
    if (!requester_global_init(&e)) {
        LS_FATALF(pd, "cannot initialize requester module: %s", my_error_cstr(&e));
        my_error_dispose(&e);
        return false;
    }
    return true;
}

static int visit_data_sources_elem(MoonVisit *mv, void *ud, int kpos, int vpos)
{
    mv->where = "'data_sources' elem";

    if (moon_visit_checktype_at(mv, "key", kpos, LUA_TSTRING) < 0) {
        return -1;
    }

    if (moon_visit_checktype_at(mv, "value", vpos, LUA_TSTRING) < 0) {
        return -1;
    }

    Priv *p = ud;

    const char *name = lua_tostring(mv->L, kpos);

    size_t lua_program_len;
    const char *lua_program = lua_tolstring(mv->L, vpos, &lua_program_len);
    if (lua_program_len != strlen(lua_program)) {
        moon_visit_err(mv, "lua program contains NUL character");
        return -1;
    }
    DSS_list_push(&p->dss_list, DSS_new(name, lua_program));

    return 1;
}

static bool perform_initial_checks(LuastatusPluginData *pd, Priv *p)
{
    size_t n = DSS_list_size(&p->dss_list);
    if (n > CONC_QUEUE_MAX_SLOTS) {
        LS_FATALF(pd, "too many keys in 'data_sources' (limit is %d)", (int) CONC_QUEUE_MAX_SLOTS);
        return false;
    }
    if (!n && p->upd_timeout == 0) {
        LS_FATALF(
            pd,
            "zero upd_timeout without any data sources is prohibited "
            "(would lead to busy-polling)");
        return false;
    }

    const char *duplicate;
    if (!DSS_list_names_unique(&p->dss_list, &duplicate)) {
        LS_FATALF(pd, "duplicate key '%s' in 'data_sources'", duplicate);
        return false;
    }

    return true;
}

static int init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    priv_init(p);

    map_ref_init(&p->map_ref, pd);

    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    if (parse_opts(p, &mv) < 0) {
        goto mverror;
    }

    // Parse make_self_pipe
    bool mkpipe = false;
    if (moon_visit_bool(&mv, -1, "make_self_pipe", &mkpipe, true) < 0) {
        goto mverror;
    }
    if (mkpipe) {
        if (ls_self_pipe_open(p->self_pipe) < 0) {
            LS_FATALF(pd, "ls_self_pipe_open: %s", ls_tls_strerror(errno));
            goto error;
        }
    }

    // Parse data_sources
    if (moon_visit_table_f(&mv, -1, "data_sources", visit_data_sources_elem, p, true) < 0) {
        goto mverror;
    }

    if (!perform_initial_checks(pd, p)) {
        goto error;
    }
    if (!init_requester(pd)) {
        goto error;
    }
    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static void append_to_lua_buf(void *ud, SAFEV segment)
{
    luaL_Buffer *b = ud;
    luaL_addlstring(b, SAFEV_ptr_UNSAFE(segment), SAFEV_len(segment));
}

static int lfunc_json_escape(lua_State *L)
{
    size_t ns;
    const char *s = luaL_checklstring(L, 1, &ns);

    SAFEV v = SAFEV_new_UNSAFE(s, ns);

    luaL_Buffer b;
    luaL_buffinit(L, &b);

    // L: ?

    escape_json_generic(append_to_lua_buf, &b, v);

    luaL_pushresult(&b); // L: ? result

    return 1;
}

static void maybe_push_escape_q_lfunc(
        LuastatusPluginData *pd,
        lua_State *L,
        const char *repl_old,
        const char *repl_new)
{
    MyError e = {0};
    if (!escape_q_make_lfunc(L, repl_old, repl_new, &e)) {
        LS_ERRF(pd, "cannot make escape-quoted Lua func: %s", my_error_cstr(&e));
        my_error_dispose(&e);
        lua_pushnil(L);
    }
}

static void register_funcs(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv;

    // L: table
    ls_self_pipe_push_luafunc(p->self_pipe, L); // L: table func
    lua_setfield(L, -2, "wake_up"); // L: table

    // L: table
    ls_pushed_timeout_push_luafunc(&p->pushed_tmo, L); // L: table func
    lua_setfield(L, -2, "push_timeout"); // L: table

    pushed_str_push_luafunc(&p->pushed_extra_params_json, L); // L: table func
    lua_setfield(L, -2, "push_extra_params_json"); // L: table

    // L: table
    lua_pushcfunction(L, lfunc_json_escape); // L: table func
    lua_setfield(L, -2, "json_escape"); // L: table

    // L: table
    maybe_push_escape_q_lfunc(pd, L, "\"", "''"); // L: table func_or_nil
    lua_setfield(L, -2, "escape_double_quoted"); // L: table

    // L: table
    maybe_push_escape_q_lfunc(pd, L, "'", "`"); // L: table func_or_nil
    lua_setfield(L, -2, "escape_single_quoted"); // L: table
}

static void report_answer(
    LuastatusPluginData *pd,
    LuastatusPluginRunFuncs funcs,
    const char *ans,
    size_t ans_len,
    int mu)
{
    ReportField fields[] = {
        /*0*/ report_field_cstr("what", "answer"),
        /*1*/ report_field_lstr("answer", ans, ans_len),
        /*2*/ {0},
    };
    size_t nfields = 2;
    if (mu >= 0) {
        fields[nfields++] = report_field_bool("mu", mu != 0);
    }
    report_generic(pd, funcs, fields, nfields);
}

static void report_error(
    LuastatusPluginData *pd,
    LuastatusPluginRunFuncs funcs,
    MyError *e)
{
    char meta_buf[16];
    if (e->meta_domain) {
        snprintf(meta_buf, sizeof(meta_buf), "%c%d", e->meta_domain, e->meta_code);
    } else {
        meta_buf[0] = '\0';
    }

    ReportField fields[] = {
        report_field_cstr("what", "error"),
        report_field_cstr("error", my_error_cstr(e)),
        report_field_cstr("meta", meta_buf),
    };
    report_generic(pd, funcs, fields, LS_ARRAY_SIZE(fields));
}

static void report_something_without_details(
    LuastatusPluginData *pd,
    LuastatusPluginRunFuncs funcs,
    const char *what)
{
    ReportField fields[] = {
        report_field_cstr("what", what),
    };
    report_generic(pd, funcs, fields, LS_ARRAY_SIZE(fields));
}

static void do_wait(
    LuastatusPluginData *pd,
    LuastatusPluginRunFuncs funcs,
    Priv *p,
    LS_TimeDelta orig_tmo)
{
    LS_TimeDelta tmo = ls_pushed_timeout_fetch(&p->pushed_tmo, orig_tmo);

    bool woken_up_by_self_pipe = false;

    int fd = p->self_pipe[0];
    if (fd < 0) {
        ls_sleep(tmo);
        goto maybe_report;
    }
    int r = ls_wait_input_on_fd(fd, tmo);
    if (r < 0) {
        LS_ERRF(pd, "do_wait: poll: %s", ls_tls_strerror(errno));
        return;
    }
    if (r > 0) {
        char dummy;
        ssize_t read_r = read(fd, &dummy, 1);
        (void) read_r;

        woken_up_by_self_pipe = true;
    }

maybe_report:
    if (woken_up_by_self_pipe) {
        report_something_without_details(pd, funcs, "self_pipe");
    } else {
        if (p->tell_about_timeout) {
            report_something_without_details(pd, funcs, "timeout");
        }
    }
}

static int lfunc_get_state_on_error(lua_State *L)
{
    lua_pushvalue(L, lua_upvalueindex(1)); // L: upvalue
    return 1;
}

static void flash_call_push_error_result(lua_State *L, SlotState state)
{
    switch (state) {
    case SLOT_STATE_ERROR_LUA_ERR:
        lua_pushstring(L, "cb_lua_error"); // L: str
        break;
    case SLOT_STATE_ERROR_PLUGIN_DONE:
        lua_pushstring(L, "plugin_exited"); // L: str
        break;
    default:
        // should not happen
        lua_pushstring(L, "?"); // L: str
        break;
    }
    lua_pushcclosure(L, lfunc_get_state_on_error, 1); // L: func
}

static int get_prompt_via_flash_call(
    LuastatusPluginData *pd,
    LuastatusPluginRunFuncs funcs,
    const LS_String *slots,
    const char *slot_states,
    size_t nslots,
    LS_String *out)
{
    Priv *p = pd->priv;

    lua_State *L = funcs.call_begin(pd->userdata);

    // L: -
    LS_ASSERT(p->lref_prompt_func != LUA_REFNIL);
    lua_rawgeti(L, LUA_REGISTRYINDEX, p->lref_prompt_func);
    // L: func
    lua_createtable(L, 0, nslots); // L: func table
    for (size_t i = 0; i < nslots; ++i) {
        SlotState state = slot_states[i];
        switch (state) {
        case SLOT_STATE_EMPTY:
        case SLOT_STATE_NIL:
            lua_pushnil(L);
            break;
        case SLOT_STATE_HAS_VALUE:
            lua_pushlstring(L, slots[i].data, slots[i].size);
            break;
        case SLOT_STATE_ERROR_LUA_ERR:
        case SLOT_STATE_ERROR_PLUGIN_DONE:
            flash_call_push_error_result(L, state);
            break;
        }
        // L: func table value
        lua_setfield(L, -2, DSS_list_get_name(&p->dss_list, i)); // L: func table
    }

    int retval;
    int rc = lua_pcall(L, 1, 1, 0);
    if (rc == 0) {
        // L: retval

        int t = lua_type(L, -1);
        if (t == LUA_TSTRING) {
            size_t res_n;
            const char *res = lua_tolstring(L, -1, &res_n);
            if (res_n) {
                ls_string_assign_b(out, res, res_n);
                retval = 1;
            } else {
                retval = 0;
            }

        } else if (t == LUA_TNIL) {
            retval = 0;

        } else {
            retval = -1;
            LS_ERRF(
                pd, "The prompt function returned %s value (expected string or nil)!",
                luaL_typename(L, -1));
        }

    } else {
        // L: error_obj

        retval = -1;

        const char *kind;
        const char *msg;
        describe_lua_err(L, rc, &kind, &msg);

        LS_ERRF(pd, "Lua error while running prompt function (see details below)!");
        LS_ERRF(pd, "Error kind: %s", kind);
        LS_ERRF(pd, "Error message: %s", msg);
    }

    funcs.call_cancel(pd->userdata);

    return retval;
}

typedef struct {
    ConcQueue q;
    LS_String *slots;
    char *slot_states;
    LS_String answer_str;
    LS_String prompt_str;
    Requester *R;
    pthread_t *tid_list;
    size_t tid_list_n;
    MyError e;
} RunData;

static void run_data_init(RunData *rd, size_t n)
{
    conc_queue_create(&rd->q, n);
    rd->slots = LS_XNEW(LS_String, n);
    for (size_t i = 0; i < n; ++i) {
        rd->slots[i] = ls_string_new_reserve(1024);
    }
    rd->slot_states = LS_XNEW(char, n);
    rd->answer_str = ls_string_new_reserve(1024);
    rd->prompt_str = ls_string_new_reserve(1024);
    rd->R = NULL;
    rd->tid_list = LS_XNEW(pthread_t, n);
    rd->tid_list_n = 0;
    rd->e = (MyError) {0};
}

static void run_data_destroy(RunData *rd, size_t n)
{
    for (size_t i = 0; i < rd->tid_list_n; ++i) {
        LS_PTH_CHECK(pthread_join(rd->tid_list[i], NULL));
    }
    free(rd->tid_list);

    conc_queue_destroy(&rd->q);

    for (size_t i = 0; i < n; ++i) {
        ls_string_free(rd->slots[i]);
    }
    free(rd->slots);

    free(rd->slot_states);

    ls_string_free(rd->answer_str);
    ls_string_free(rd->prompt_str);
    if (rd->R) {
        requester_destroy(rd->R);
    }
    my_error_dispose(&rd->e);
}

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    size_t n = DSS_list_size(&p->dss_list);

    RunData rd;
    run_data_init(&rd, n);

    if (!(rd.R = requester_new(&p->cnx_settings, pd, &rd.e))) {
        LS_FATALF(pd, "cannot create requester: %s", my_error_cstr(&rd.e));
        goto fail;
    }

    {
        MapValue_PI *val = map_ref_load(p->map_ref);
        LS_PTH_CHECK(pthread_mutex_lock(&val->mtx));
        for (size_t i = 0; i < n; ++i) {
            pthread_t tid;
            const char *name = DSS_list_get_name(&p->dss_list, i);
            const char *lua_program = DSS_list_get_lua_program(&p->dss_list, i);
            if (mini_luastatus_run(lua_program, name, &rd.q, i, pd, &tid)) {
                rd.tid_list[rd.tid_list_n++] = tid;
            }
            DSS_list_free_lua_program(&p->dss_list, i);
        }
        LS_PTH_CHECK(pthread_mutex_unlock(&val->mtx));
    }

    if (p->greet) {
        LS_DEBUGF(pd, "greeting");
        report_something_without_details(pd, funcs, "hello");
    }

    do_wait(pd, funcs, p, LS_TD_ZERO);
    LS_TimeDelta upd_timeout_TD = ls_double_to_TD_or_die(p->upd_timeout);

    for (;;) {
        LS_DEBUGF(pd, "fetching updates");

        // fetch updates
        if (n != 0) {
            (void) conc_queue_fetch_updates(&rd.q, rd.slots, rd.slot_states);
        }

        LS_DEBUGF(pd, "generatig prompt");

        // generate prompt
        int prompt_rc = get_prompt_via_flash_call(pd, funcs, rd.slots, rd.slot_states, n, &rd.prompt_str);
        if (prompt_rc < 0) {
            report_something_without_details(pd, funcs, "prompt_error");
        } else if (prompt_rc == 0) {
            continue;
        }

        LS_DEBUGF(pd, "making request");

        // make request
        char *pushed_EPJ = pushed_str_fetch(&p->pushed_extra_params_json);
        bool req_ok = requester_make_request(
            rd.R,
            pushed_EPJ ? pushed_EPJ : p->extra_params_json,
            SAFEV_new_UNSAFE(rd.prompt_str.data, rd.prompt_str.size),
            &rd.answer_str,
            &rd.e);
        free(pushed_EPJ);

        LS_DEBUGF(pd, "calling widget's cb");

        // call widget
        if (req_ok) {
            int mu = -1;
            if (p->report_mu) {
                CONC_QUEUE_MASK mask = conc_queue_peek_at_updates(&rd.q);
                mu = mask ? 1 : 0;
            }
            report_answer(pd, funcs, rd.answer_str.data, rd.answer_str.size, mu);

        } else {
            report_error(pd, funcs, &rd.e);
            my_error_clear(&rd.e);
        }

        LS_DEBUGF(pd, "sleeping");

        // sleep
        do_wait(pd, funcs, p, upd_timeout_TD);
    }

fail:
    run_data_destroy(&rd, n);
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .run = run,
    .register_funcs = register_funcs,
    .destroy = destroy,
};
