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

#include "runner.h"
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <dlfcn.h>

#include "include/plugin_data_v1.h"

#include "libls/ls_panic.h"
#include "libls/ls_xallocf.h"
#include "libls/ls_alloc_utils.h"
#include "libls/ls_getenv_r.h"
#include "libls/ls_lua_compat.h"

#include "librunshell/runshell.h"
#include "libwidechar/libwidechar.h"

#include "external_context.h"

#include "config.generated.h"

// Logging macros.
#define FATALF(Runner_, ...)    my_sayf(Runner_, LUASTATUS_LOG_FATAL,    __VA_ARGS__)
#define ERRF(Runner_, ...)      my_sayf(Runner_, LUASTATUS_LOG_ERR,      __VA_ARGS__)
#define WARNF(Runner_, ...)     my_sayf(Runner_, LUASTATUS_LOG_WARN,     __VA_ARGS__)
#define INFOF(Runner_, ...)     my_sayf(Runner_, LUASTATUS_LOG_INFO,     __VA_ARGS__)
#define VERBOSEF(Runner_, ...)  my_sayf(Runner_, LUASTATUS_LOG_VERBOSE,  __VA_ARGS__)
#define DEBUGF(Runner_, ...)    my_sayf(Runner_, LUASTATUS_LOG_DEBUG,    __VA_ARGS__)
#define TRACEF(Runner_, ...)    my_sayf(Runner_, LUASTATUS_LOG_TRACE,    __VA_ARGS__)

#define LOCK_L(W_)   LS_PTH_CHECK(pthread_mutex_lock(&(W_)->L_mtx))
#define UNLOCK_L(W_) LS_PTH_CHECK(pthread_mutex_unlock(&(W_)->L_mtx))

enum { SAYF_BUF_SIZE = 1024 };

typedef struct {
    // The interface loaded from this plugin's .so file.
    LuastatusPluginIface_v1 iface;

    // An allocated zero-terminated string with plugin name, as specified in widget's
    // /widget.plugin/ string.
    char *name;

    // A handle returned from /dlopen/ for this plugin's .so file.
    void *dlhandle;
} Plugin;

typedef struct {
    // An initialized plugin.
    Plugin plugin;

    // /plugin/'s data for this widget.
    LuastatusPluginData_v1 data;

    // This widget's Lua interpreter instance.
    lua_State *L;

    // A mutex guarding /L/.
    pthread_mutex_t L_mtx;

    // Lua reference (in /L/'s registry) to this widget's /widget.cb/ function.
    int lref_cb;

    // Lua reference (in /L/'s registry) to this widget's /widget.event/ function (is
    // /LUA_REFNIL/ if the latter is /nil/).
    int lref_event;

    // An allocated widget's name.
    char *name;
} Widget;

struct Runner {
    ExternalContext ectx;
    Widget widget;
    RunnerCallback callback;
    void *callback_ud;
};

// This function exists because /dlerror()/ may return /NULL/ even if /dlsym()/ returned /NULL/.
static inline const char *safe_dlerror(void)
{
    const char *err = dlerror();
    return err ? err : "(no error, but the symbol is NULL)";
}

static inline void safe_vsnprintf(char *buf, size_t nbuf, const char *fmt, va_list vl)
{
    if (vsnprintf(buf, nbuf, fmt, vl) < 0) {
        buf[0] = '\0';
    }
}

static void my_sayf(Runner *runner, int level, const char *fmt, ...)
{
    char buf[SAYF_BUF_SIZE];
    va_list vl;
    va_start(vl, fmt);
    safe_vsnprintf(buf, sizeof(buf), fmt, vl);
    va_end(vl);

    runner->ectx->sayf(
        runner->ectx->userdata,
        level,
        "%s", buf
    );
}

static void external_sayf(void *userdata, int level, const char *fmt, ...)
{
    Runner *runner = userdata;

    char buf[SAYF_BUF_SIZE];
    va_list vl;
    va_start(vl, fmt);
    safe_vsnprintf(buf, sizeof(buf), fmt, vl);
    va_end(vl);

    runner->ectx->sayf(
        runner->ectx->userdata,
        level,
        "[%s] %s", runner->widget.name, buf
    );
}

// Returns a pointer to the value of the entry with key /key/; or creates a new entry with the given
// key and /NULL/ value, and returns a pointer to that value.
static void **map_get(void *userdata, const char *key)
{
    LS_ASSERT(key != NULL);

    Runner *runner = userdata;

    TRACEF(runner, "map_get(userdata=%p, key='%s')", userdata, key);

    return runner->ectx->map_get(runner->ectx->userdata, key);
}

static bool plugin_load(Runner *runner, Plugin *p, const char *filename, const char *name)
{
    p->dlhandle = NULL;
    p->name = ls_xstrdup(name);

    DEBUGF(runner, "loading plugin from file '%s'", filename);

    (void) dlerror(); // clear last error
    if (!(p->dlhandle = dlopen(filename, RTLD_NOW | RTLD_LOCAL))) {
        ERRF(runner, "dlopen: %s: %s", filename, safe_dlerror());
        goto error;
    }
    int *p_lua_ver = dlsym(p->dlhandle, "LUASTATUS_PLUGIN_LUA_VERSION_NUM");
    if (!p_lua_ver) {
        ERRF(runner, "dlsym: LUASTATUS_PLUGIN_LUA_VERSION_NUM: %s", safe_dlerror());
        goto error;
    }
    if (*p_lua_ver != LUA_VERSION_NUM) {
        ERRF(runner, "plugin '%s' was compiled with LUA_VERSION_NUM=%d and luastatus with %d",
             filename, *p_lua_ver, LUA_VERSION_NUM);
        goto error;
    }
    LuastatusPluginIface_v1 *p_iface = dlsym(p->dlhandle, "luastatus_plugin_iface_v1");
    if (!p_iface) {
        ERRF(runner, "dlsym: luastatus_plugin_iface_v1: %s", safe_dlerror());
        goto error;
    }
    p->iface = *p_iface;
    DEBUGF(runner, "plugin successfully loaded");
    return true;

error:
    if (p->dlhandle) {
        dlclose(p->dlhandle);
    }
    free(p->name);
    return false;
}

static bool plugin_load_by_name(Runner *runner, Plugin *p, const char *name)
{
    if ((strchr(name, '/'))) {
        return plugin_load(runner, p, name, name);
    } else {
        char *filename = ls_xallocf("%s/plugin-%s.so", LUASTATUS_PLUGINS_DIR, name);
        bool r = plugin_load(runner, p, filename, name);
        free(filename);
        return r;
    }
}

static void plugin_unload(Plugin *p)
{
    free(p->name);
    dlclose(p->dlhandle);
}

static lua_State *xnew_lua_state(void)
{
    lua_State *L = luaL_newstate();
    if (!L) {
        LS_PANIC("luaL_newstate() failed: out of memory?");
    }
    return L;
}

// Returns a string representation of an error object located at the position /pos/ of /L/'s stack.
static inline const char *get_lua_error_msg(lua_State *L, int pos)
{
    const char *msg = lua_tostring(L, pos);
    return msg ? msg : "(error object cannot be converted to string)";
}

// Checks a /lua_*/ call that returns a /LUA_*/ error code, performed on a Lua interpreter instance
// /L/. /ret/ is the return value of the call.
//
// If /ret/ is /0/, returns /true/; otherwise, logs the error and returns /false/.
static bool check_lua_call(Runner *runner, lua_State *L, int ret)
{
    const char *prefix;
    switch (ret) {
    case 0:
        return true;
    case LUA_ERRRUN:
    case LUA_ERRSYNTAX:
    case LUA_ERRMEM:  // Lua itself produces a meaningful error message in this case
    case LUA_ERRFILE: // ditto
        prefix = "(lua) ";
        break;
    case LUA_ERRERR:
        prefix = "(lua) error while running error handler: ";
        break;
#ifdef LUA_ERRGCMM
    // Introduced in Lua 5.2 and removed in in Lua 5.4.
    case LUA_ERRGCMM:
        prefix = "(lua) error while running __gc metamethod: ";
        break;
#endif
    default:
        prefix = "unknown Lua error code (please report!), message is: ";
    }
    // L: ? error
    ERRF(runner, "%s%s", prefix, get_lua_error_msg(L, -1));
    lua_pop(L, 1);
    // L: ?
    return false;
}

// The Lua error handler that gets called whenever an error occurs inside a chunk called with
// /do_lua_call/.
//
// Currently, it returns (to /check_lua_call/) the traceback of the error.
static int l_error_handler(lua_State *L)
{
    // L: error
    lua_getglobal(L, LUA_DBLIBNAME); // L: error debug
    lua_getfield(L, -1, "traceback"); // L: error debug traceback
    lua_pushstring(L, get_lua_error_msg(L, 1)); // L: error debug traceback msg
    lua_pushinteger(L, 2); // L: error debug traceback msg level
    lua_call(L, 2, 1); // L: error debug result
    return 1;
}

// Similar to /lua_call/, but expects an error handler to be at the bottom of /L/'s stack, runs the
// chunk with that error handler, and logs the error message, if any.
//
// Returns /true/ on success, /false/ on failure.
static inline bool do_lua_call(Runner *runner, lua_State *L, int nargs, int nresults)
{
    return check_lua_call(runner, L, lua_pcall(L, nargs, nresults, 1));
}

// Replacement for Lua's /os.exit()/: a simple /exit()/ used by Lua is not thread-safe in Linux.
static int l_os_exit(lua_State *L)
{
    int code = luaL_optinteger(L, 1, /*default value*/ EXIT_SUCCESS);
    fflush(stdout);
    fflush(stderr);
    _exit(code);
}

// Replacement for Lua's /os.getenv()/: a simple /getenv()/ used by Lua is not guaranteed by POSIX
// to be thread-safe.
static int l_os_getenv(lua_State *L)
{
    const char *r = ls_getenv_r(luaL_checkstring(L, 1));
    if (r) {
        lua_pushstring(L, r);
    } else {
        ls_lua_pushfail(L);
    }
    return 1;
}

// Replacement for Lua's /os.setlocale()/: this thing is inherently thread-unsafe.
static int l_os_setlocale(lua_State *L)
{
    ls_lua_pushfail(L);
    return 1;
}

// Implementation of /luastatus.require_plugin()/. Expects a single upvalue: an initially empty
// table that will be used as a registry of loaded Lua plugins.
static int l_require_plugin(lua_State *L)
{
    const char *arg = luaL_checkstring(L, 1);
    if ((strchr(arg, '/'))) {
        return luaL_argerror(L, 1, "plugin name contains a slash");
    }
    lua_pushvalue(L, lua_upvalueindex(1)); // L: ? table
    lua_getfield(L, -1, arg); // L: ? table value
    if (!lua_isnil(L, -1)) {
        return 1;
    }
    lua_pop(L, 1); // L: ? table

    char *filename = ls_xallocf("%s/%s.lua", LUASTATUS_LUA_PLUGINS_DIR, arg);
    int r = luaL_loadfile(L, filename);
    free(filename);
    if (r != 0) {
        return lua_error(L);
    }

    // L: ? table chunk
    lua_call(L, 0, 1); // L: ? table result
    lua_pushvalue(L, -1); // L: ? table result result
    lua_setfield(L, -3, arg); // L: ? table result
    return 1;
}

static void inject_libs_replacements(lua_State *L)
{
    lua_getglobal(L, "os"); // L: ? os

    lua_pushcfunction(L, l_os_exit); // L: ? os l_os_exit
    lua_setfield(L, -2, "exit"); // L: ? os

    lua_pushcfunction(L, l_os_getenv); // L: ? os l_os_getenv
    lua_setfield(L, -2, "getenv"); // L: ? os

    lua_pushcfunction(L, l_os_setlocale); // L: ? os l_os_setlocale
    lua_setfield(L, -2, "setlocale"); // L: ? os

    bool is_lua51 = ls_lua_is_lua51(L);
    lua_pushcfunction(
        L,
        is_lua51 ? runshell_l_os_execute_lua51ver : runshell_l_os_execute);
    // L: ? os os_execute_func
    lua_setfield(L, -2, "execute"); // L: ? os

    lua_pop(L, 1); // L: ?
}

static void inject_luastatus_module(lua_State *L)
{
    lua_createtable(L, 0, 4); // L: ? table

    // ========== require_plugin ==========
    lua_newtable(L); // L: ? table table
    lua_pushcclosure(L, l_require_plugin, 1); // L: ? table l_require_plugin
    lua_setfield(L, -2, "require_plugin"); // L: ? table

    // ========== execute ==========
    lua_pushcfunction(L, runshell_l_os_execute); // L: ? table cfunction
    lua_setfield(L, -2, "execute"); // L: ? table

    // ========== libwidechar ==========
    lua_newtable(L); // L: ? table table
    libwidechar_register_lua_funcs(L); // L: ? table table
    lua_setfield(L, -2, "libwidechar"); // L: ? table

    lua_setglobal(L, "luastatus"); // L: ?
}

// 1. Replaces some of the functions in the standard library with our thread-safe counterparts.
// 2. Registers the /luastatus/ module (just creates a global table actually) except for the
//    /luastatus.plugin/ and /luastatus.barlib/ submodules (created later).
static void inject_libs(lua_State *L)
{
    inject_libs_replacements(L);
    inject_luastatus_module(L);
}

// Inspects the 'plugin' field of /w/'s /widget/ table; the /widget/ table is assumed to be on top
// of /w.L/'s stack. The stack itself is not changed by this function.
static bool widget_init_inspect_plugin(Runner *runner)
{
    Widget *w = &runner->widget;
    lua_State *L = w->L;
    // L: ? widget
    lua_getfield(L, -1, "plugin"); // L: ? widget plugin
    if (!lua_isstring(L, -1)) {
        ERRF(runner, "'widget.plugin': expected string, found %s", luaL_typename(L, -1));
        return false;
    }
    if (!plugin_load_by_name(runner, &w->plugin, lua_tostring(L, -1))) {
        ERRF(runner, "cannot load plugin '%s'", lua_tostring(L, -1));
        return false;
    }
    lua_pop(L, 1); // L: ? widget
    return true;
}

// Inspects the 'cb' field of /w/'s /widget/ table; the /widget/ table is assumed to be on top
// of /w.L/'s stack. The stack itself is not changed by this function.
static bool widget_init_inspect_cb(Runner *runner)
{
    Widget *w = &runner->widget;
    lua_State *L = w->L;
    // L: ? widget
    lua_getfield(L, -1, "cb"); // L: ? widget plugin
    if (!lua_isfunction(L, -1)) {
        ERRF(runner, "'widget.cb': expected function, found %s", luaL_typename(L, -1));
        return false;
    }
    w->lref_cb = luaL_ref(L, LUA_REGISTRYINDEX); // L: ? widget
    return true;
}

// Inspects the 'event' field of /w/'s /widget/ table; the /widget/ table is assumed to be on top
// of /w.L/'s stack. The stack itself is not changed by this function.
static bool widget_init_inspect_event(Runner *runner)
{
    Widget *w = &runner->widget;
    lua_State *L = w->L;
    // L: ? widget
    lua_getfield(L, -1, "event"); // L: ? widget event
    switch (lua_type(L, -1)) {
    case LUA_TNIL:
    case LUA_TFUNCTION:
        w->lref_event = luaL_ref(L, LUA_REGISTRYINDEX); // L: ? widget
        return true;
    default:
        ERRF(runner, "'widget.event': expected function or nil, found %s", luaL_typename(L, -1));
        return false;
    }
}

// Inspects the 'opts' field of /w/'s /widget/ table; the /widget/ table is assumed to be on top
// of /w.L/'s stack.
//
// Pushes either the value of 'opts', or a new empty table if it is absent, onto the stack.
static bool widget_init_inspect_push_opts(Runner *runner)
{
    Widget *w = &runner->widget;
    lua_State *L = w->L;
    lua_getfield(L, -1, "opts"); // L: ? widget opts
    switch (lua_type(L, -1)) {
    case LUA_TTABLE:
        return true;
    case LUA_TNIL:
        lua_pop(L, 1); // L: ? widget
        lua_newtable(L); // L: ? widget table
        return true;
    default:
        ERRF(runner, "'widget.opts': expected table or nil, found %s", luaL_typename(L, -1));
        return false;
    }
}

static bool widget_init(Runner *runner, const char *name, const char *code)
{
    Widget *w = &runner->widget;
    w->L = xnew_lua_state();
    LS_PTH_CHECK(pthread_mutex_init(&w->L_mtx, NULL));
    w->name = ls_xstrdup(name);
    bool plugin_loaded = false;

    DEBUGF(runner, "initializing widget [%s]", name);

    luaL_openlibs(w->L);
    // w->L: -
    inject_libs(w->L); // w->L: -
    lua_pushcfunction(w->L, l_error_handler); // w->L: l_error_handler

    DEBUGF(runner, "running widget [%s]", name);

    char *chunk_name = ls_xallocf("data source [%s]", name);
    bool loaded_ok = check_lua_call(
        runner,
        w->L,
        luaL_loadbuffer(w->L, code, strlen(code), chunk_name));
    free(chunk_name);
    if (!loaded_ok) {
        goto error;
    }
    // w->L: l_error_handler chunk
    if (!do_lua_call(runner, w->L, 0, 0)) {
        goto error;
    }
    // w->L: l_error_handler

    lua_getglobal(w->L, "widget"); // w->L: l_error_handler widget
    if (!lua_istable(w->L, -1)) {
        ERRF(runner, "'widget': expected table, found %s", luaL_typename(w->L, -1));
        goto error;
    }

    if (!widget_init_inspect_plugin(runner)) {
        goto error;
    }
    plugin_loaded = true;
    if (!widget_init_inspect_cb(runner) ||
        !widget_init_inspect_event(runner) ||
        !widget_init_inspect_push_opts(runner))
    {
        goto error;
    }
    // w->L: l_error_handler widget opts

    w->data = (LuastatusPluginData_v1) {
        .userdata = runner,
        .sayf = external_sayf,
        .map_get = map_get,
    };

    if (w->plugin.iface.init(&w->data, w->L) == LUASTATUS_ERR) {
        ERRF(runner, "plugin's init() failed");
        goto error;
    }
    LS_ASSERT(lua_gettop(w->L) == 3); // w->L: l_error_handler widget opts
    lua_pop(w->L, 2); // w->L: l_error_handler

    DEBUGF(runner, "widget successfully initialized");
    return true;

error:
    lua_close(w->L);
    LS_PTH_CHECK(pthread_mutex_destroy(&w->L_mtx));
    free(w->name);
    if (plugin_loaded) {
        plugin_unload(&w->plugin);
    }
    return false;
}

static void widget_destroy(Widget *w)
{
    w->plugin.iface.destroy(&w->data);
    plugin_unload(&w->plugin);
    lua_close(w->L);
    LS_PTH_CHECK(pthread_mutex_destroy(&w->L_mtx));
    free(w->name);
}

static void register_funcs(Runner *runner)
{
    Widget *w = &runner->widget;
    lua_State *L = w->L;

    // L: ?
    lua_getglobal(L, "luastatus"); // L: ? luastatus

    if (!lua_istable(L, -1)) {
        WARNF(
            runner,
            "widget [%s]: 'luastatus' is not a table anymore, will not register "
            "barlib/plugin functions",
            w->name);
        goto done;
    }
    if (w->plugin.iface.register_funcs) {
        lua_newtable(L); // L: ? luastatus table

        int old_top = lua_gettop(L);
        (void) old_top;
        w->plugin.iface.register_funcs(&w->data, L); // L: ? luastatus table
        LS_ASSERT(lua_gettop(L) == old_top);

        lua_setfield(L, -2, "plugin"); // L: ? luastatus
    }

done:
    lua_pop(L, 1); // L: ?
}

static lua_State *plugin_call_begin(void *userdata)
{
    Runner *runner = userdata;
    TRACEF(runner, "plugin_call_begin(userdata=%p)", userdata);

    Widget *w = &runner->widget;
    LOCK_L(w);

    lua_State *L = w->L;
    LS_ASSERT(lua_gettop(L) == 1); // w->L: l_error_handler
    lua_rawgeti(L, LUA_REGISTRYINDEX, w->lref_cb); // w->L: l_error_handler cb
    return L;
}

static void plugin_call_end(void *userdata)
{
    Runner *runner = userdata;
    TRACEF(runner, "plugin_call_end(userdata=%p)", userdata);

    Widget *w = &runner->widget;
    lua_State *L = w->L;
    LS_ASSERT(lua_gettop(L) == 3); // L: l_error_handler cb data
    bool r = do_lua_call(runner, L, 1, 1);

    if (r) {
        runner->callback(runner->callback_ud, REASON_NEW_DATA, L);
    } else {
        runner->callback(runner->callback_ud, REASON_LUA_ERROR, NULL);
    }

    lua_settop(L, 1); // L: l_error_handler

    UNLOCK_L(w);
}

static void plugin_call_cancel(void *userdata)
{
    Runner *runner = userdata;
    TRACEF(runner, "plugin_call_cancel(userdata=%p)", userdata);

    Widget *w = &runner->widget;
    lua_settop(w->L, 1); // w->L: l_error_handler
    UNLOCK_L(w);
}

lua_State *runner_event_begin(Runner *runner)
{
    Widget *w = &runner->widget;

    LOCK_L(w);

    lua_State *L = w->L;
    LS_ASSERT(lua_gettop(L) == 1); // L: l_error_handler
    lua_rawgeti(L, LUA_REGISTRYINDEX, w->lref_event); // L: l_error_handler event

    return L;
}

RunnerEventResult runner_event_end(Runner *runner)
{
    RunnerEventResult ret;

    Widget *w = &runner->widget;

    lua_State *L = w->L;
    LS_ASSERT(lua_gettop(L) == 3); // L: l_error_handler event arg

    if (w->lref_event == LUA_REFNIL) {
        lua_pop(L, 2); // L: l_error_handler
        ret = EVT_RESULT_NO_HANDLER;
    } else {
        bool is_ok = do_lua_call(runner, L, 1, 0); // L: l_error_handler
        ret = is_ok ? EVT_RESULT_OK : EVT_RESULT_LUA_ERROR;
    }

    UNLOCK_L(w);

    return ret;
}

void runner_run(Runner *runner)
{
    Widget *w = &runner->widget;

    DEBUGF(runner, "widget [%s] is now running", w->name);

    w->plugin.iface.run(&w->data, (LuastatusPluginRunFuncs_v1) {
        .call_begin  = plugin_call_begin,
        .call_end    = plugin_call_end,
        .call_cancel = plugin_call_cancel,
    });
    WARNF(runner, "plugin's run() for widget [%s] has returned", w->name);

    runner->callback(runner->callback_ud, REASON_PLUGIN_EXITED, NULL);
}

Runner *runner_new(
    const char *name,
    const char *code,
    ExternalContext ectx,
    RunnerCallback callback,
    void *callback_ud)
{
    Runner *runner = LS_XNEW(Runner, 1);
    *runner = (Runner) {
        .ectx = ectx,
        .callback = callback,
        .callback_ud = callback_ud,
    };
    if (!widget_init(runner, name, code)) {
        free(runner);
        return NULL;
    }
    register_funcs(runner);
    return runner;
}

void runner_destroy(Runner *runner)
{
    widget_destroy(&runner->widget);
    free(runner);
}
