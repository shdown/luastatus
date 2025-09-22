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

#include "mini_luastatus.h"

#include <assert.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <dlfcn.h>

#include "include/plugin_data.h"
#include "include/common.h"

#include "libls/ls_alloc_utils.h"
#include "libls/ls_compdep.h"
#include "libls/ls_getenv_r.h"
#include "libls/ls_tls_ebuf.h"
#include "libls/ls_xallocf.h"

#include "config.generated.h"

#include "conc_queue.h"
#include "describe_lua_err.h"
#include "external_context.h"

#define FATALF(Myself_, ...)    sayf(Myself_, LUASTATUS_LOG_FATAL,    __VA_ARGS__)
#define ERRF(Myself_, ...)      sayf(Myself_, LUASTATUS_LOG_ERR,      __VA_ARGS__)
#define WARNF(Myself_, ...)     sayf(Myself_, LUASTATUS_LOG_WARN,     __VA_ARGS__)
#define INFOF(Myself_, ...)     sayf(Myself_, LUASTATUS_LOG_INFO,     __VA_ARGS__)
#define VERBOSEF(Myself_, ...)  sayf(Myself_, LUASTATUS_LOG_VERBOSE,  __VA_ARGS__)
#define DEBUGF(Myself_, ...)    sayf(Myself_, LUASTATUS_LOG_DEBUG,    __VA_ARGS__)
#define TRACEF(Myself_, ...)    sayf(Myself_, LUASTATUS_LOG_TRACE,    __VA_ARGS__)

typedef struct {
    LuastatusPluginIface_v1 iface;
    void *dlhandle;
} Plugin;

static inline Plugin plugin_new(void)
{
    return (Plugin) {
        .iface = {0},
        .dlhandle = NULL,
    };
}

typedef struct {
    Plugin plugin;
    bool plugin_iface_inited;
    LuastatusPluginData_v1 data;
    lua_State *L;
    int lref_cb;
} DataSource;

static inline DataSource data_source_new(void)
{
    return (DataSource) {
        .plugin = plugin_new(),
        .plugin_iface_inited = false,
        .data = {0},
        .L = NULL,
        .lref_cb = LUA_REFNIL,
    };
}

// forward declaration
struct Myself;
typedef struct Myself Myself;

typedef struct {
    char is_data_source;
    Myself *myself;
} MyUserData;

struct Myself {
    DataSource data_source;

    ExternalContext ectx;
    MyUserData ud_for_me;
    MyUserData ud_for_data_source;

    ConcQueue *q;

    size_t q_idx;

    char *name;
};

static const char *loglevel_names[] = {
    [LUASTATUS_LOG_FATAL]   = "fatal",
    [LUASTATUS_LOG_ERR]     = "error",
    [LUASTATUS_LOG_WARN]    = "warning",
    [LUASTATUS_LOG_INFO]    = "info",
    [LUASTATUS_LOG_VERBOSE] = "verbose",
    [LUASTATUS_LOG_DEBUG]   = "debug",
    [LUASTATUS_LOG_TRACE]   = "trace",
};

// This function exists because /dlerror()/ may return /NULL/ even if /dlsym()/ returned /NULL/.
static inline const char *safe_dlerror(void)
{
    const char *err = dlerror();
    return err ? err : "(no error, but the symbol is NULL)";
}

static void common_vsayf(void *userdata, int level, const char *fmt, va_list vl)
{
    MyUserData *my_ud = userdata;
    Myself *myself = my_ud->myself;

    char is_data_source = my_ud->is_data_source;
    ExternalContext ectx = myself->ectx;

    char buf[1024];
    if (vsnprintf(buf, sizeof(buf), fmt, vl) < 0) {
        buf[0] = '\0';
    }

    const char *prefix = is_data_source ? " (data source) " : "";
    ectx->sayf(
        ectx->userdata,
        level,
        "%s%s: %s: %s",
        myself->name,
        prefix,
        loglevel_names[level],
        buf);
}

static inline LS_ATTR_PRINTF(3, 4)
void sayf(Myself *myself, int level, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    common_vsayf(&myself->ud_for_me, level, fmt, vl);
    va_end(vl);
}

static void external_sayf(void *userdata, int level, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    common_vsayf(userdata, level, fmt, vl);
    va_end(vl);
}

static Myself *myself_new(const char *name, ConcQueue *q, size_t q_idx, ExternalContext ectx)
{
    Myself *myself = LS_XNEW(Myself, 1);
    *myself = (Myself) {
        .data_source = data_source_new(),
        .ectx = ectx,
        .q = q,
        .q_idx = q_idx,
        .name = ls_xstrdup(name),
    };
    myself->ud_for_me = (MyUserData) {
        .is_data_source = 0,
        .myself = myself,
    };
    myself->ud_for_data_source = (MyUserData) {
        .is_data_source = 1,
        .myself = myself,
    };
    return myself;
}

static void myself_done(Myself *myself)
{
    conc_queue_update_slot(myself->q, myself->q_idx, NULL, 0, 'D');
}

static void myself_do_report_error(Myself *myself)
{
    conc_queue_update_slot(myself->q, myself->q_idx, NULL, 0, 'E');
}

static void myself_do_report_result(Myself *myself, lua_State *L)
{
    int t = lua_type(L, -1);
    if (t == LUA_TSTRING) {
        size_t res_n;
        const char *res = lua_tolstring(L, -1, &res_n);
        conc_queue_update_slot(myself->q, myself->q_idx, res, res_n, 'y');
    } else if (t == LUA_TNIL) {
        conc_queue_update_slot(myself->q, myself->q_idx, NULL, 0, 'n');
    } else {
        ERRF(myself, "cb returned %s value (expected string or nil)", lua_typename(L, t));
        myself_do_report_error(myself);
    }
}

static void **map_get(void *userdata, const char *key)
{
    MyUserData *my_ud = userdata;
    Myself *myself = my_ud->myself;

    ExternalContext ectx = myself->ectx;
    return ectx->map_get(ectx->userdata, key);
}

static bool plugin_load(Myself *myself, Plugin *p, const char *filename)
{
    *p = (Plugin) {0};

    DEBUGF(myself, "loading plugin from file '%s'", filename);

    (void) dlerror(); // clear last error
    if (!(p->dlhandle = dlopen(filename, RTLD_NOW | RTLD_LOCAL))) {
        ERRF(myself, "dlopen: %s: %s", filename, safe_dlerror());
        goto error;
    }
    int *p_lua_ver = dlsym(p->dlhandle, "LUASTATUS_PLUGIN_LUA_VERSION_NUM");
    if (!p_lua_ver) {
        ERRF(myself, "dlsym: LUASTATUS_PLUGIN_LUA_VERSION_NUM: %s", safe_dlerror());
        goto error;
    }
    if (*p_lua_ver != LUA_VERSION_NUM) {
        ERRF(
            myself,
            "plugin '%s' was compiled with LUA_VERSION_NUM=%d and mini_luastatus with %d",
            filename,
            *p_lua_ver,
            LUA_VERSION_NUM);
        goto error;
    }
    LuastatusPluginIface_v1 *p_iface = dlsym(p->dlhandle, "luastatus_plugin_iface_v1");
    if (!p_iface) {
        ERRF(myself, "dlsym: luastatus_plugin_iface_v1: %s", safe_dlerror());
        goto error;
    }
    p->iface = *p_iface;
    DEBUGF(myself, "plugin successfully loaded");
    return true;

error:
    return false;
}

static bool plugin_load_by_name(Myself *myself, Plugin *p, const char *name)
{
    if ((strchr(name, '/'))) {
        return plugin_load(myself, p, name);
    } else {
        char *filename = ls_xallocf("%s/plugin-%s.so", LUASTATUS_PLUGINS_DIR, name);
        bool r = plugin_load(myself, p, filename);
        free(filename);
        return r;
    }
}

static bool check_lua_call(Myself *myself, lua_State *L, int ret)
{
    if (ret == 0) {
        return true;
    }
    // L: ? error

    const char *kind;
    const char *msg;
    describe_lua_err(L, ret, &kind, &msg);

    ERRF(myself, "%s: %s", kind, msg);

    lua_pop(L, 1); // L: ?
    return false;
}

static inline bool do_lua_call(Myself *myself, lua_State *L, int nargs, int nresults)
{
    return check_lua_call(myself, L, lua_pcall(L, nargs, nresults, 0));
}

// Replacement for Lua's /os.exit()/.
static int l_os_exit(lua_State *L)
{
    return luaL_error(L, "You are a data source, you can't terminate the whole process");
}

// Replacement for Lua's /os.getenv()/: a simple /getenv()/ used by Lua is not guaranteed by POSIX
// to be thread-safe.
static int l_os_getenv(lua_State *L)
{
    const char *r = ls_getenv_r(luaL_checkstring(L, 1));
    if (r) {
        lua_pushstring(L, r);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

// Replacement for Lua's /os.setlocale()/: this thing is inherently thread-unsafe.
static int l_os_setlocale(lua_State *L)
{
    lua_pushnil(L);
    return 1;
}

// 1. Replaces some of the functions in the standard library with our thread-safe counterparts.
// 2. Registers the /luastatus/ module (just creates a global table actually) except for the
//    /luastatus.plugin/ submodule (created later).
static void inject_libs(lua_State *L)
{
    lua_getglobal(L, "os"); // L: ? os

    lua_pushcfunction(L, l_os_exit); // L: ? os l_os_exit
    lua_setfield(L, -2, "exit"); // L: ? os

    lua_pushcfunction(L, l_os_getenv); // L: ? os l_os_getenv
    lua_setfield(L, -2, "getenv"); // L: ? os

    lua_pushcfunction(L, l_os_setlocale); // L: ? os l_os_setlocale
    lua_setfield(L, -2, "setlocale"); // L: ? os

    lua_pop(L, 1); // L: ?

    lua_newtable(L); // L: ? table
    lua_setglobal(L, "luastatus"); // L: ?
}

// Inspects the 'plugin' field of /ds/'s /data_source/ table; the /data_source/ table is assumed to be on top
// of /ds.L/'s stack. The stack itself is not changed by this function.
static bool data_source_load_inspect_plugin(Myself *myself, DataSource *ds)
{
    lua_State *L = ds->L;
    // L: ? data_source
    lua_getfield(L, -1, "plugin"); // L: ? data_source plugin
    if (!lua_isstring(L, -1)) {
        ERRF(myself, "'LL_data_source.plugin': expected string, found %s", luaL_typename(L, -1));
        return false;
    }
    if (!plugin_load_by_name(myself, &ds->plugin, lua_tostring(L, -1))) {
        ERRF(myself, "cannot load plugin '%s'", lua_tostring(L, -1));
        return false;
    }
    lua_pop(L, 1); // L: ? data_source
    return true;
}

// Inspects the 'cb' field of /ds/'s /data_source/ table; the /data_source/ table is assumed to be on top
// of /ds.L/'s stack. The stack itself is not changed by this function.
static bool data_source_load_inspect_cb(Myself *myself, DataSource *ds)
{
    lua_State *L = ds->L;
    // L: ? data_source
    lua_getfield(L, -1, "cb"); // L: ? data_source plugin
    if (!lua_isfunction(L, -1)) {
        ERRF(myself, "'LL_data_source.cb': expected function, found %s", luaL_typename(L, -1));
        return false;
    }
    ds->lref_cb = luaL_ref(L, LUA_REGISTRYINDEX); // L: ? data_source
    return true;
}

// Inspects the 'opts' field of /ds/'s /data_source/ table; the /data_source/ table is assumed to be on top
// of /ds.L/'s stack.
//
// Pushes either the value of 'opts', or a new empty table if it is absent, onto the stack.
static bool data_source_load_inspect_push_opts(Myself *myself, DataSource *ds)
{
    lua_State *L = ds->L;
    lua_getfield(L, -1, "opts"); // L: ? data_source opts
    switch (lua_type(L, -1)) {
    case LUA_TTABLE:
        return true;
    case LUA_TNIL:
        lua_pop(L, 1); // L: ? data_source
        lua_newtable(L); // L: ? data_source table
        return true;
    default:
        ERRF(myself, "'LL_data_source.opts': expected table or nil, found %s", luaL_typename(L, -1));
        return false;
    }
}

static bool data_source_load(Myself *myself, DataSource *ds, const char *lua_program)
{
    *ds = (DataSource) {0};

    if (!(ds->L = luaL_newstate())) {
        ERRF(myself, "luaL_newstate() failed");
        goto error;
    }

    DEBUGF(myself, "creating data source");

    luaL_openlibs(ds->L);
    // ds->L: -
    inject_libs(ds->L); // ds->L: -

    DEBUGF(myself, "running data source");
    if (!check_lua_call(myself, ds->L, luaL_loadstring(ds->L, lua_program))) {
        goto error;
    }
    // ds->L: chunk
    if (!do_lua_call(myself, ds->L, 0, 0)) {
        goto error;
    }

    lua_getglobal(ds->L, "LL_data_source"); // ds->L: data_source
    if (!lua_istable(ds->L, -1)) {
        ERRF(myself, "'LL_data_source': expected table, found %s", luaL_typename(ds->L, -1));
        goto error;
    }

    if (!data_source_load_inspect_plugin(myself, ds) ||
        !data_source_load_inspect_cb(myself, ds) ||
        !data_source_load_inspect_push_opts(myself, ds))
    {
        goto error;
    }
    // ds->L: data_source opts

    ds->data = (LuastatusPluginData_v1) {
        .userdata = &myself->ud_for_data_source,
        .sayf = external_sayf,
        .map_get = map_get,
    };

    if (ds->plugin.iface.init(&ds->data, ds->L) == LUASTATUS_ERR) {
        ERRF(myself, "plugin's init() failed");
        goto error;
    }
    ds->plugin_iface_inited = true;
    assert(lua_gettop(ds->L) == 2); // ds->L: data_source opts
    lua_pop(ds->L, 2); // ds->L: -

    DEBUGF(myself, "data_source successfully created");
    return true;

error:
    return false;
}

static void register_funcs(Myself *myself, lua_State *L, DataSource *ds)
{
    // L: ?
    lua_getglobal(L, "luastatus"); // L: ? luastatus

    if (!lua_istable(L, -1)) {
        WARNF(
            myself,
            "'luastatus' is not a table anymore, will not register "
            "plugin functions");
        goto done;
    }

    if (ds && ds->plugin.iface.register_funcs) {
        lua_newtable(L); // L: ? luastatus table

        int old_top = lua_gettop(L);
        (void) old_top;
        ds->plugin.iface.register_funcs(&ds->data, L); // L: ? luastatus table
        assert(lua_gettop(L) == old_top);

        lua_setfield(L, -2, "plugin"); // L: ? luastatus
    }

done:
    lua_pop(L, 1); // L: ?
}

static lua_State *plugin_call_begin(void *userdata)
{
    MyUserData *my_ud = userdata;
    Myself *myself = my_ud->myself;
    DataSource *ds = &myself->data_source;

    lua_State *L = ds->L;
    assert(lua_gettop(L) == 0); // ds->L: 0
    lua_rawgeti(L, LUA_REGISTRYINDEX, ds->lref_cb); // ds->L: cb
    return L;
}

static void plugin_call_end(void *userdata)
{
    MyUserData *my_ud = userdata;
    Myself *myself = my_ud->myself;
    DataSource *ds = &myself->data_source;

    lua_State *L = ds->L;
    assert(lua_gettop(L) == 2); // L: cb data
    bool r = do_lua_call(myself, L, 1, 1);

    if (r) {
        // L: result
        myself_do_report_result(myself, L);
        lua_settop(L, 0); // L: -
    } else {
        // L: -
        myself_do_report_error(myself);
    }
}

static void plugin_call_cancel(void *userdata)
{
    MyUserData *my_ud = userdata;
    Myself *myself = my_ud->myself;
    DataSource *ds = &myself->data_source;

    lua_settop(ds->L, 0); // ds->L: -
}

static bool myself_realize(Myself *myself, const char *lua_program)
{
    DataSource *ds = &myself->data_source;
    if (!data_source_load(myself, ds, lua_program)) {
        return false;
    }
    register_funcs(myself, ds->L, ds);
    return true;
}

static void plugin_destroy(Plugin *p)
{
    if (p->dlhandle) {
        dlclose(p->dlhandle);
    }
}

static void data_source_destroy(DataSource *ds)
{
    if (ds->plugin_iface_inited) {
        ds->plugin.iface.destroy(&ds->data);
    }
    if (ds->L) {
        lua_close(ds->L);
    }
    plugin_destroy(&ds->plugin);
}

static void myself_destroy(Myself *myself)
{
    data_source_destroy(&myself->data_source);
    free(myself->name);
    free(myself);
}

static void *data_source_thread(void *arg)
{
    Myself *myself = arg;
    DataSource *ds = &myself->data_source;

    DEBUGF(myself, "thread for data source is running");

    ds->plugin.iface.run(&ds->data, (LuastatusPluginRunFuncs_v1) {
        .call_begin  = plugin_call_begin,
        .call_end    = plugin_call_end,
        .call_cancel = plugin_call_cancel,
    });
    WARNF(myself, "plugin's run() for data source has returned");

    myself_done(myself);
    myself_destroy(myself);

    return NULL;
}

bool mini_luastatus_run(
    const char *lua_program,
    const char *name,
    ConcQueue *q,
    size_t q_idx,
    ExternalContext ectx,
    pthread_t *out_pid)
{
    Myself *myself = myself_new(name, q, q_idx, ectx);

    if (!myself_realize(myself, lua_program)) {
        goto error;
    }

    int err_num = pthread_create(out_pid, NULL, data_source_thread, myself);
    if (err_num) {
        ERRF(myself, "pthread_create: %s", ls_tls_strerror(err_num));
        goto error;
    }
    return true;
error:
    myself_destroy(myself);
    return false;
}
