/*
 * Copyright (C) 2015-2020  luastatus developers
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

#include <assert.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include <dlfcn.h>
#include <unistd.h>

#include "include/barlib_data.h"
#include "include/plugin_data.h"
#include "include/common.h"

#include "libls/alloc_utils.h"
#include "libls/compdep.h"
#include "libls/getenv_r.h"
#include "libls/string_.h"
#include "libls/algo.h"
#include "libls/panic.h"

#include "config.generated.h"

// Logging macros.
#define FATALF(...)    sayf(LUASTATUS_LOG_FATAL,    __VA_ARGS__)
#define ERRF(...)      sayf(LUASTATUS_LOG_ERR,      __VA_ARGS__)
#define WARNF(...)     sayf(LUASTATUS_LOG_WARN,     __VA_ARGS__)
#define INFOF(...)     sayf(LUASTATUS_LOG_INFO,     __VA_ARGS__)
#define VERBOSEF(...)  sayf(LUASTATUS_LOG_VERBOSE,  __VA_ARGS__)
#define DEBUGF(...)    sayf(LUASTATUS_LOG_DEBUG,    __VA_ARGS__)
#define TRACEF(...)    sayf(LUASTATUS_LOG_TRACE,    __VA_ARGS__)

// These ones are implemented as macros so that /LS_PTH_CHECK()/ calls receive the correct line
// they are called at.
#define LOCK_B()   LS_PTH_CHECK(pthread_mutex_lock(&barlib.set_mtx))
#define UNLOCK_B() LS_PTH_CHECK(pthread_mutex_unlock(&barlib.set_mtx))

#define LOCK_L(W_)   LS_PTH_CHECK(pthread_mutex_lock(&(W_)->L_mtx))
#define UNLOCK_L(W_) LS_PTH_CHECK(pthread_mutex_unlock(&(W_)->L_mtx))

#define LOCK_E(W_)   LS_PTH_CHECK(pthread_mutex_lock(widget_event_L_mtx(W_)))
#define UNLOCK_E(W_) LS_PTH_CHECK(pthread_mutex_unlock(widget_event_L_mtx(W_)))

typedef struct {
    // The interface loaded from this plugin's .so file.
    LuastatusPluginIface_v1 iface;

    // An allocated zero-terminated string with plugin name, as specified in widget's
    // /widget.plugin/ string.
    char *name;

    // A handle returned from /dlopen/ for this plugin's .so file.
    void *dlhandle;
} Plugin;

// If any step of widget's initialization fails, the widget is not removed from the /widgets/
// buffer, but is, instead, unloaded and becomes *stillborn*; barlib's /set_error()/ is called on
// it, and a separate "runner" thread simply does not get spawned for it.
//
// However, barlib's /event_watcher()/ may still report events on such a widget.
// Possible solutions to this are:
//   1. Allow the event watcher's /call_begin/ function (/ew_call_begin/) to return /NULL/ to tell
//      the event watcher that we are not interested in this event, and that it should be skipped.
//      Complicates the API and event watcher's logic.
//   2. Initialize each stillborn widget's /L/ with an empty Lua state, and provide it to the event
//      watcher each time it generates an event on this widget.
//   3. If there is at least one stillborn widget, initialize the *separate state* (see below), and
//      provide /sepstate.L/ to the event watcher. A slight benefit over the second variant is that
//      only one extra initialized Lua state is required.
//
// We choose the third one, and thus require stillborn widgets to have:
//   1. /sepstate_event/ field set to /true/ so that /ew_call_begin/ and /ew_call_end/ functions
//      would operate on /sepstate/'s Lua state and mutex guarding it, instead of widget's ones
//      (which are not initialized in the case of a stillborn widget);
//   2. /lref_event/ field set to /LUA_REFNIL/ so that /ew_call_end/ function would simply discard
//      the object generated by barlib's event watcher.

typedef struct {
    // Normal: an initialized plugin.
    // Stillborn: undefined.
    Plugin plugin;

    // Normal: /plugin/'s data for this widget.
    // Stillborn: undefined.
    LuastatusPluginData_v1 data;

    // Normal: this widget's Lua interpreter instance.
    // Stillborn: /NULL/ (used to check if the widget is stillborn).
    lua_State *L;

    // Normal: a mutex guarding /L/.
    // Stillborn: undefined.
    pthread_mutex_t L_mtx;

    // Normal: Lua reference (in /L/'s registry) to this widget's /widget.cb/ function.
    // Stillborn: undefined.
    int lref_cb;

    // Normal:
    //   if /sepstate_event/ is false, Lua reference (in /L/'s registry) to this widget's
    //     /widget.event/ function (is /LUA_REFNIL/ if the latter is /nil/);
    //   if /sepstate_event/ is true, Lua reference (in /sepstate.L/'s registry) to the compiled
    //      /widget.event/ function of this widget.
    // Stillborn: /LUA_REFNIL/.
    int lref_event;

    // Normal: whether /lref_event/ is a reference in /sepstate.L/'s registry, as opposed to
    // /L/'s one.
    // Stillborn: /true/.
    bool sepstate_event;

    // Normal: an allocated zero-terminated string with widget's file name.
    // Stillborn: undefined.
    char *filename;
} Widget;

static const char *loglevel_names[] = {
    [LUASTATUS_LOG_FATAL]   = "fatal",
    [LUASTATUS_LOG_ERR]     = "error",
    [LUASTATUS_LOG_WARN]    = "warning",
    [LUASTATUS_LOG_INFO]    = "info",
    [LUASTATUS_LOG_VERBOSE] = "verbose",
    [LUASTATUS_LOG_DEBUG]   = "debug",
    [LUASTATUS_LOG_TRACE]   = "trace",
};

// Current log level. May only be changed once, when parsing command-line arguments.
static int loglevel = LUASTATUS_LOG_INFO;

static struct {
    // The interface loaded from this barlib's .so file.
    LuastatusBarlibIface_v1 iface;

    // This barlib's data.
    LuastatusBarlibData_v1 data;

    // A mutex guarding calls to /iface.set()/ and /iface.set_error()/.
    pthread_mutex_t set_mtx;

    // A handle returned from /dlopen/ for this barlib's .so file.
    void *dlhandle;
} barlib;

// These two are initially (explicitly) set to /NULL/ and /0/ correspondingly, so that the
// destruction function (/widgets_destroy()/) can be invoked at any time (that is, before or after
// their initialization with actual values, not in the middle of it).
//
// This requires a little care with initialization; /widgets_init()/ should be used for it.
static Widget *widgets = NULL;
static size_t nwidgets = 0;

// This "separate state" thing serves two purposes:
//   1. If a widget has a /widget.event/ variable of string type, it is compiled in /sepstate.L/ Lua
//      interpreter instance as a function; a reference to it is stored in that widget's
//      /lref_event/ field; and the /sepstate_event/ field of that widget is set to /true/.
//   2. As has been already described above, /sepstate.L/ is provided to barlib's /event_watcher()/
//      each time it attempts to generate an event on a stillborn widget; the event object is then
//      simply discarded.
static struct {
    // Separate state's Lua interpreter instance. Initially is (explicitly) set to /NULL/, which
    // indicates that the separate state was not initialized yet.
    lua_State *L;

    // A mutex guarding /L/.
    pthread_mutex_t L_mtx;
} sepstate = {.L = NULL};

// See DOCS/design/map_get.md
//
// Basically, it is a string-to-pointer mapping used by plugins and barlibs for synchronization.

typedef struct MapEntry {
    void *value;
    struct MapEntry *next;
    char key[]; // zero-terminated
} MapEntry;

static struct {
    MapEntry *top;

    // Whether the map is frozen after all plugins and widgets have been initialized.
    bool frozen;
} map = {.top = NULL, .frozen = false};

// This function exists because /dlerror()/ may return /NULL/ even if /dlsym()/ returned /NULL/.
static inline const char *safe_dlerror(void)
{
    const char *err = dlerror();
    return err ? err : "(no error, but the symbol is NULL)";
}

// Returns a log level number by its name /str/, or returns /LUASTATUS_LOG_LAST/ if no such log
// level was found.
static int loglevel_fromstr(const char *str)
{
    for (size_t i = 0; i < LS_ARRAY_SIZE(loglevel_names); ++i) {
        assert(loglevel_names[i]); // a hole in enumeration?
        if (strcmp(str, loglevel_names[i]) == 0) {
            return i;
        }
    }
    return LUASTATUS_LOG_LAST;
}

// The generic logging function: generates a log message with level /level/ from a given /subsystem/
// (either a plugin or a barlib name; or /NULL/, which means the message is from the luastatus
// program itself) using the format string /fmt/ and variable arguments supplied as /vl/, as if with
// /vsnprintf(<unspecified>, fmt, vl)/.
static void common_vsayf(int level, const char *subsystem, const char *fmt, va_list vl)
{
    if (level > loglevel) {
        return;
    }

    char buf[1024];
    if (vsnprintf(buf, sizeof(buf), fmt, vl) < 0) {
        buf[0] = '\0';
    }

    if (subsystem) {
        fprintf(stderr, "luastatus: (%s) %s: %s\n", subsystem, loglevel_names[level], buf);
    } else {
        fprintf(stderr, "luastatus: %s: %s\n", loglevel_names[level], buf);
    }
}

// The "internal" logging function: generates a log message from the luastatus program itself with
// level /level/ using the format string /fmt/ and the variable arguments supplied as /.../, as if
// with /vsnprintf(<unspecified>, fmt, <... variable arguments>)/.
static inline LS_ATTR_PRINTF(2, 3)
void sayf(int level, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    common_vsayf(level, NULL, fmt, vl);
    va_end(vl);
}

// The "external" logging function: generates a log message from the subsystem denoted by /userdata/
// with level /level/ using the format string /fmt/ and the variable arguments supplied as /.../, as
// if with /vsnprintf(<unspecified>, fmt, <... variable arguments>/.
static void external_sayf(void *userdata, int level, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    if (userdata) {
        Widget *w = userdata;
        char who[1024];
        snprintf(who, sizeof(who), "%s@%s", w->plugin.name, w->filename);
        common_vsayf(level, who, fmt, vl);
    } else {
        common_vsayf(level, "barlib", fmt, vl);
    }
    va_end(vl);
}

// Returns a pointer to the value of the entry with key /key/; or creates a new entry with the given
// key and /NULL/ value, and returns a pointer to that value.
static void **map_get(void *userdata, const char *key)
{
    TRACEF("map_get(userdata=%p, key='%s')", userdata, key);

    if (map.frozen) {
        FATALF("map_get() is called after the map has been frozen");
        abort();
    }

    for (MapEntry *e = map.top; e; e = e->next)
        if (strcmp(key, e->key) == 0)
            return &e->value;

    // Not found; create a new entry with /NULL/ value.
    size_t nkey = strlen(key);
    MapEntry *e = ls_xmalloc(sizeof(MapEntry) + nkey + 1, 1);
    e->value = NULL;
    e->next = map.top;
    memcpy(e->key, key, nkey + 1);
    map.top = e;

    return &e->value;
}

static void map_destroy(void)
{
    MapEntry *e = map.top;
    while (e) {
        MapEntry *next = e->next;
        free(e);
        e = next;
    }
}

// Loads /barlib/ from a file /filename/ and initializes with options /opts/ and the number of
// widgets /nwidgets/ (a global variable).
static bool barlib_init(const char *filename, const char *const *opts)
{
    barlib.dlhandle = NULL;
    LS_PTH_CHECK(pthread_mutex_init(&barlib.set_mtx, NULL));

    DEBUGF("initializing barlib from file '%s'", filename);

    (void) dlerror(); // clear last error
    if (!(barlib.dlhandle = dlopen(filename, RTLD_NOW | RTLD_LOCAL))) {
        ERRF("dlopen: %s: %s", filename, safe_dlerror());
        goto error;
    }
    int *p_lua_ver = dlsym(barlib.dlhandle, "LUASTATUS_BARLIB_LUA_VERSION_NUM");
    if (!p_lua_ver) {
        ERRF("dlsym: LUASTATUS_BARLIB_LUA_VERSION_NUM: %s", safe_dlerror());
        goto error;
    }
    if (*p_lua_ver != LUA_VERSION_NUM) {
        ERRF("barlib '%s' was compiled with LUA_VERSION_NUM=%d and luastatus with %d",
             filename, *p_lua_ver, LUA_VERSION_NUM);
        goto error;
    }
    LuastatusBarlibIface_v1 *p_iface = dlsym(barlib.dlhandle, "luastatus_barlib_iface_v1");
    if (!p_iface) {
        ERRF("dlsym: luastatus_barlib_iface_v1: %s", safe_dlerror());
        goto error;
    }
    barlib.iface = *p_iface;
    barlib.data = (LuastatusBarlibData_v1) {
        .userdata = NULL,
        .sayf = external_sayf,
        .map_get = map_get,
    };

    if (barlib.iface.init(&barlib.data, opts, nwidgets) == LUASTATUS_ERR) {
        ERRF("barlib's init() failed");
        goto error;
    }
    DEBUGF("barlib successfully initialized");
    return true;

error:
    if (barlib.dlhandle) {
        dlclose(barlib.dlhandle);
    }
    LS_PTH_CHECK(pthread_mutex_destroy(&barlib.set_mtx));
    return false;
}

// The result is same to calling /barlib_init(<filename>, opts)/, where /<filename>/ is the file
// name guessed for name /name/.
static bool barlib_init_by_name(const char *name, const char *const *opts)
{
    if ((strchr(name, '/'))) {
        return barlib_init(name, opts);
    } else {
        LSString filename = ls_string_newz_from_f("%s/barlib-%s.so", LUASTATUS_BARLIBS_DIR, name);
        bool r = barlib_init(filename.data, opts);
        ls_string_free(filename);
        return r;
    }
}

static void barlib_destroy(void)
{
    barlib.iface.destroy(&barlib.data);
    dlclose(barlib.dlhandle);
    LS_PTH_CHECK(pthread_mutex_destroy(&barlib.set_mtx));
}

static bool plugin_load(Plugin *p, const char *filename, const char *name)
{
    p->dlhandle = NULL;
    p->name = ls_xstrdup(name);

    DEBUGF("loading plugin from file '%s'", filename);

    (void) dlerror(); // clear last error
    if (!(p->dlhandle = dlopen(filename, RTLD_NOW | RTLD_LOCAL))) {
        ERRF("dlopen: %s: %s", filename, safe_dlerror());
        goto error;
    }
    int *p_lua_ver = dlsym(p->dlhandle, "LUASTATUS_PLUGIN_LUA_VERSION_NUM");
    if (!p_lua_ver) {
        ERRF("dlsym: LUASTATUS_PLUGIN_LUA_VERSION_NUM: %s", safe_dlerror());
        goto error;
    }
    if (*p_lua_ver != LUA_VERSION_NUM) {
        ERRF("plugin '%s' was compiled with LUA_VERSION_NUM=%d and luastatus with %d",
             filename, *p_lua_ver, LUA_VERSION_NUM);
        goto error;
    }
    LuastatusPluginIface_v1 *p_iface = dlsym(p->dlhandle, "luastatus_plugin_iface_v1");
    if (!p_iface) {
        ERRF("dlsym: luastatus_plugin_iface_v1: %s", safe_dlerror());
        goto error;
    }
    p->iface = *p_iface;
    DEBUGF("plugin successfully loaded");
    return true;

error:
    if (p->dlhandle) {
        dlclose(p->dlhandle);
    }
    free(p->name);
    return false;
}

static bool plugin_load_by_name(Plugin *p, const char *name)
{
    if ((strchr(name, '/'))) {
        return plugin_load(p, name, name);
    } else {
        LSString filename = ls_string_newz_from_f("%s/plugin-%s.so", LUASTATUS_PLUGINS_DIR, name);
        bool r = plugin_load(p, filename.data, name);
        ls_string_free(filename);
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
        FATALF("luaL_newstate() failed: out of memory?");
        abort();
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
static bool check_lua_call(lua_State *L, int ret)
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
    // first introduced in Lua 5.2
    case LUA_ERRGCMM:
        prefix = "(lua) error while running __gc metamethod: ";
        break;
#endif
    default:
        prefix = "unknown Lua error code (please report!), message is: ";
    }
    // L: ? error
    ERRF("%s%s", prefix, get_lua_error_msg(L, -1));
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
static inline bool do_lua_call(lua_State *L, int nargs, int nresults)
{
    return check_lua_call(L, lua_pcall(L, nargs, nresults, 1));
}

// Replacement for Lua's /os.exit()/: a simple /exit()/ used by Lua is not thread-safe in Linux.
static int l_os_exit(lua_State *L)
{
    int code = luaL_optinteger(L, 1, /*default value*/ EXIT_SUCCESS);
    fflush(NULL);
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

// Implementation of /luastatus.require_plugin()/. Expects a single upvalue: an initially empty
// table that will be used as a registry of loaded Lua plugins.
static int l_require_plugin(lua_State *L)
{
    const char *arg = luaL_checkstring(L, 1);
    if ((strchr(arg, '/'))) {
        return luaL_error(L, "plugin name contains a slash");
    }
    lua_pushvalue(L, lua_upvalueindex(1)); // L: ? table
    lua_getfield(L, -1, arg); // L: ? table value
    if (!lua_isnil(L, -1)) {
        return 1;
    }
    lua_pop(L, 1); // L: ? table

    LSString filename = ls_string_newz_from_f("%s/%s.lua", LUASTATUS_PLUGINS_DIR, arg);
    int r = luaL_loadfile(L, filename.data);
    ls_string_free(filename);
    if (r != 0) {
        return lua_error(L);
    }

    // L: ? table chunk
    lua_call(L, 0, 1); // L: ? table result
    lua_pushvalue(L, -1); // L: ? table result result
    lua_setfield(L, -3, arg); // L: ? table result
    return 1;
}

// 1. Replaces some of the functions in the standard library with our thread-safe counterparts.
// 2. Registers the /luastatus/ module (just creates a global table actually) except for the
//    /luastatus.plugin/ and /luastatus.barlib/ submodules (created later).
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

    lua_createtable(L, 0, 1); // L: ? table

    lua_newtable(L); // L: ? table table
    lua_pushcclosure(L, l_require_plugin, 1); // L: ? table l_require_plugin
    lua_setfield(L, -2, "require_plugin"); // L: ? table

    lua_setglobal(L, "luastatus"); // L: ?
}

static void sepstate_maybe_init(void)
{
    if (sepstate.L) {
        // already initialized
        return;
    }
    sepstate.L = xnew_lua_state();
    luaL_openlibs(sepstate.L);
    inject_libs(sepstate.L);
    lua_pushcfunction(sepstate.L, l_error_handler); // sepstate.L: l_error_handler
    LS_PTH_CHECK(pthread_mutex_init(&sepstate.L_mtx, NULL));
}

static void sepstate_maybe_destroy(void)
{
    if (!sepstate.L) {
        // hasn't been initialized
        return;
    }
    lua_close(sepstate.L);
    LS_PTH_CHECK(pthread_mutex_destroy(&sepstate.L_mtx));
}

// Inspects the 'plugin' field of /w/'s /widget/ table; the /widget/ table is assumed to be on top
// of /w.L/'s stack. The stack itself is not changed by this function.
static bool widget_init_inspect_plugin(Widget *w)
{
    lua_State *L = w->L;
    // L: ? widget
    lua_getfield(L, -1, "plugin"); // L: ? widget plugin
    if (!lua_isstring(L, -1)) {
        ERRF("'widget.plugin': expected string, found %s", luaL_typename(L, -1));
        return false;
    }
    if (!plugin_load_by_name(&w->plugin, lua_tostring(L, -1))) {
        ERRF("cannot load plugin '%s'", lua_tostring(L, -1));
        return false;
    }
    lua_pop(L, 1); // L: ? widget
    return true;
}

// Inspects the 'cb' field of /w/'s /widget/ table; the /widget/ table is assumed to be on top
// of /w.L/'s stack. The stack itself is not changed by this function.
static bool widget_init_inspect_cb(Widget *w)
{
    lua_State *L = w->L;
    // L: ? widget
    lua_getfield(L, -1, "cb"); // L: ? widget plugin
    if (!lua_isfunction(L, -1)) {
        ERRF("'widget.cb': expected function, found %s", luaL_typename(L, -1));
        return false;
    }
    w->lref_cb = luaL_ref(L, LUA_REGISTRYINDEX); // L: ? widget
    return true;
}

// Inspects the 'event' field of /w/'s /widget/ table; the /widget/ table is assumed to be on top
// of /w.L/'s stack. The stack itself is not changed by this function.
static bool widget_init_inspect_event(Widget *w, const char *filename)
{
    lua_State *L = w->L;
    // L: ? widget
    lua_getfield(L, -1, "event"); // L: ? widget event
    switch (lua_type(L, -1)) {
    case LUA_TNIL:
    case LUA_TFUNCTION:
        w->lref_event = luaL_ref(L, LUA_REGISTRYINDEX); // L: ? widget
        w->sepstate_event = false;
        return true;
    case LUA_TSTRING:
        {
            sepstate_maybe_init();

            size_t ncode;
            const char *code = lua_tolstring(w->L, -1, &ncode);

            LSString chunkname = ls_string_newz_from_f("widget.event of %s", filename);
            bool r = check_lua_call(
                sepstate.L, luaL_loadbuffer(sepstate.L, code, ncode, chunkname.data));
            ls_string_free(chunkname);
            if (!r) {
                return false;
            }

            // sepstate.L: ? chunk
            w->lref_event = luaL_ref(sepstate.L, LUA_REGISTRYINDEX); // sepstate.L: ?
            w->sepstate_event = true;
            lua_pop(L, 1); // L: ? widget
            return true;
        }
    default:
        ERRF("'widget.event': expected function, nil, or string, found %s", luaL_typename(L, -1));
        return false;
    }
}

// Inspects the 'opts' field of /w/'s /widget/ table; the /widget/ table is assumed to be on top
// of /w.L/'s stack.
//
// Pushes either the value of 'opts', or a new empty table if it is absent, onto the stack.
static bool widget_init_inspect_push_opts(Widget *w)
{
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
        ERRF("'widget.opts': expected table or nil, found %s", luaL_typename(L, -1));
        return false;
    }
}

static bool widget_init(Widget *w, const char *filename)
{
    w->L = xnew_lua_state();
    LS_PTH_CHECK(pthread_mutex_init(&w->L_mtx, NULL));
    w->filename = ls_xstrdup(filename);
    bool plugin_loaded = false;

    DEBUGF("initializing widget '%s'", filename);

    luaL_openlibs(w->L);
    // w->L: -
    inject_libs(w->L); // w->L: -
    lua_pushcfunction(w->L, l_error_handler); // w->L: l_error_handler

    DEBUGF("running file '%s'", filename);
    if (!check_lua_call(w->L, luaL_loadfile(w->L, filename))) {
        goto error;
    }
    // w->L: l_error_handler chunk
    if (!do_lua_call(w->L, 0, 0)) {
        goto error;
    }
    // w->L: l_error_handler

    lua_getglobal(w->L, "widget"); // w->L: l_error_handler widget
    if (!lua_istable(w->L, -1)) {
        ERRF("'widget': expected table, found %s", luaL_typename(w->L, -1));
        goto error;
    }

    if (!widget_init_inspect_plugin(w)) {
        goto error;
    }
    plugin_loaded = true;
    if (!widget_init_inspect_cb(w) ||
        !widget_init_inspect_event(w, filename) ||
        !widget_init_inspect_push_opts(w))
    {
        goto error;
    }
    // w->L: l_error_handler widget opts

    w->data = (LuastatusPluginData_v1) {
        .userdata = w,
        .sayf = external_sayf,
        .map_get = map_get,
    };

    if (w->plugin.iface.init(&w->data, w->L) == LUASTATUS_ERR) {
        ERRF("plugin's init() failed");
        goto error;
    }
    assert(lua_gettop(w->L) == 3); // w->L: l_error_handler widget opts
    lua_pop(w->L, 2); // w->L: l_error_handler

    DEBUGF("widget successfully initialized");
    return true;

error:
    lua_close(w->L);
    LS_PTH_CHECK(pthread_mutex_destroy(&w->L_mtx));
    free(w->filename);
    if (plugin_loaded) {
        plugin_unload(&w->plugin);
    }
    return false;
}

static void widget_init_stillborn(Widget *w)
{
    sepstate_maybe_init();
    w->L = NULL;
    w->lref_event = LUA_REFNIL;
    w->sepstate_event = true;
}

static inline bool widget_is_stillborn(Widget *w)
{
    return !w->L;
}

// Returns the Lua interpreter instance for the /widget.event/ function of a widget /w/.
static inline lua_State *widget_event_lua_state(Widget *w)
{
    return w->sepstate_event ? sepstate.L : w->L;
}

// Returns a pointer to the mutex guarding the Lua interpreter instance for the /widget.event/
// function of a widget /w/.
static inline pthread_mutex_t *widget_event_L_mtx(Widget *w)
{
    return w->sepstate_event ? &sepstate.L_mtx : &w->L_mtx;
}

static inline size_t widget_index(Widget *w)
{
    return w - widgets;
}

static void widget_destroy(Widget *w)
{
    if (!widget_is_stillborn(w)) {
        w->plugin.iface.destroy(&w->data);
        plugin_unload(&w->plugin);
        lua_close(w->L);
        LS_PTH_CHECK(pthread_mutex_destroy(&w->L_mtx));
        free(w->filename);
    }
}

// Registers /barlib/'s functions at /L/.
// If /w/ is not /NULL/, also registers /w->plugin/'s functions at /L/.
static void register_funcs(lua_State *L, Widget *w)
{
    // L: ?
    lua_getglobal(L, "luastatus"); // L: ? luastatus

    if (!lua_istable(L, -1)) {
        assert(w);
        assert(!widget_is_stillborn(w));

        WARNF("widget '%s': 'luastatus' is not a table anymore, will not register "
              "barlib/plugin functions",
              w->filename);
        goto done;
    }
    if (barlib.iface.register_funcs) {
        lua_newtable(L); // L: ? luastatus table

        int old_top = lua_gettop(L);
        (void) old_top;
        barlib.iface.register_funcs(&barlib.data, L); // L: ? luastatus table
        assert(lua_gettop(L) == old_top);

        lua_setfield(L, -2, "barlib"); // L: ? luastatus
    }
    if (w && w->plugin.iface.register_funcs) {
        lua_newtable(L); // L: ? luastatus table

        int old_top = lua_gettop(L);
        (void) old_top;
        w->plugin.iface.register_funcs(&w->data, L); // L: ? luastatus table
        assert(lua_gettop(L) == old_top);

        lua_setfield(L, -2, "plugin"); // L: ? luastatus
    }

done:
    lua_pop(L, 1); // L: ?
}

// Initializes the /widgets/ and /nwidgets/ global variables from the given list of file names:
// sets /nwidgets/, allocates /widgets/, initialized all the widgets, and makes ones whose
// initialization failed stillborn.
static void widgets_init(char *const *filenames, size_t nfilenames)
{
    nwidgets = nfilenames;
    widgets = LS_XNEW(Widget, nwidgets);
    for (size_t i = 0; i < nwidgets; ++i) {
        if (!widget_init(&widgets[i], filenames[i])) {
            ERRF("cannot load widget '%s'", filenames[i]);
            widget_init_stillborn(&widgets[i]);
        }
    }
}

static void widgets_destroy(void)
{
    for (size_t i = 0; i < nwidgets; ++i) {
        widget_destroy(&widgets[i]);
    }
    free(widgets);
}

// Should be invoked whenever the barlib reports a fatal error.
static LS_ATTR_NORETURN
void fatal_error_reported(void)
{
    fflush(NULL);
    _exit(EXIT_FAILURE);
}

// Invokes /barlib/'s /set_error()/ method on the widget with index /widget_idx/ and performs all
// the error-checking required.
//
// Does not do any locking/unlocking.
static void set_error_unlocked(size_t widget_idx)
{
    if (barlib.iface.set_error(&barlib.data, widget_idx) == LUASTATUS_ERR) {
        FATALF("barlib's set_error() reported fatal error");
        fatal_error_reported();
    }
}

static lua_State *plugin_call_begin(void *userdata)
{
    TRACEF("plugin_call_begin(userdata=%p)", userdata);

    Widget *w = userdata;
    LOCK_L(w);

    lua_State *L = w->L;
    assert(lua_gettop(L) == 1); // w->L: l_error_handler
    lua_rawgeti(L, LUA_REGISTRYINDEX, w->lref_cb); // w->L: l_error_handler cb
    return L;
}

static void plugin_call_end(void *userdata)
{
    TRACEF("plugin_call_end(userdata=%p)", userdata);

    Widget *w = userdata;
    lua_State *L = w->L;
    assert(lua_gettop(L) == 3); // L: l_error_handler cb data
    bool r = do_lua_call(L, 1, 1);
    LOCK_B();
    size_t widget_idx = widget_index(w);
    if (r) {
        // L: l_error_handler result
        switch (barlib.iface.set(&barlib.data, L, widget_idx)) {
        case LUASTATUS_OK:
            // L: l_error_handler result
            break;
        case LUASTATUS_NONFATAL_ERR:
            // L: l_error_handler ?
            set_error_unlocked(widget_idx);
            break;
        case LUASTATUS_ERR:
            // L: l_error_handler ?
            FATALF("barlib's set() reported fatal error");
            fatal_error_reported();
            break;
        }
        lua_settop(L, 1); // L: l_error_handler
    } else {
        // L: l_error_handler
        set_error_unlocked(widget_idx);
    }
    UNLOCK_B();
    UNLOCK_L(w);
}

static void plugin_call_cancel(void *userdata)
{
    TRACEF("plugin_call_cancel(userdata=%p)", userdata);

    Widget *w = userdata;
    lua_settop(w->L, 1); // w->L: l_error_handler
    UNLOCK_L(w);
}

static lua_State *ew_call_begin(void *userdata, size_t widget_idx)
{
    TRACEF("ew_call_begin(userdata=%p, widget_idx=%zu)", userdata, widget_idx);

    assert(widget_idx < nwidgets);
    Widget *w = &widgets[widget_idx];
    LOCK_E(w);

    lua_State *L = widget_event_lua_state(w);
    assert(lua_gettop(L) == 1); // L: l_error_handler
    lua_rawgeti(L, LUA_REGISTRYINDEX, w->lref_event); // L: l_error_handler event
    return L;
}

static void ew_call_end(void *userdata, size_t widget_idx)
{
    TRACEF("ew_call_end(userdata=%p, widget_idx=%zu)", userdata, widget_idx);

    assert(widget_idx < nwidgets);
    Widget *w = &widgets[widget_idx];
    lua_State *L = widget_event_lua_state(w);
    assert(lua_gettop(L) == 3); // L: l_error_handler event arg
    if (w->lref_event == LUA_REFNIL) {
        lua_pop(L, 2); // L: l_error_handler
    } else {
        if (!do_lua_call(L, 1, 0)) {
            // L: l_error_handler
            LOCK_B();
            set_error_unlocked(widget_idx);
            UNLOCK_B();
        }
        // L: l_error_handler
    }
    UNLOCK_E(w);
}

static void ew_call_cancel(void *userdata, size_t widget_idx)
{
    TRACEF("ew_call_cancel(userdata=%p, widget_idx=%zu)", userdata, widget_idx);

    assert(widget_idx < nwidgets);
    Widget *w = &widgets[widget_idx];
    lua_State *L = widget_event_lua_state(w);
    lua_settop(L, 1); // L: l_error_handler
    UNLOCK_E(w);
}

// Each thread spawned for a widget runs this function. /arg/ is a pointer to the widget.
static void *widget_thread(void *arg)
{
    Widget *w = arg;
    DEBUGF("thread for widget '%s' is running", w->filename);

    w->plugin.iface.run(&w->data, (LuastatusPluginRunFuncs_v1) {
        .call_begin  = plugin_call_begin,
        .call_end    = plugin_call_end,
        .call_cancel = plugin_call_cancel,
    });
    WARNF("plugin's run() for widget '%s' has returned", w->filename);

    LOCK_B();
    set_error_unlocked(widget_index(w));
    UNLOCK_B();

    return NULL;
}

static void prepare_signals(void)
{
    // We do not want to terminate on a write to a dead pipe.
    struct sigaction sa = {.sa_flags = SA_RESTART, .sa_handler = SIG_IGN};
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGPIPE, &sa, NULL) < 0) {
        perror("luastatus: sigaction: SIGPIPE");
    }
}

static void print_usage(void)
{
    fprintf(stderr, "USAGE: luastatus -b barlib [-B barlib_option [-B ...]] [-l loglevel] [-e] "
                    "widget.lua [widget2.lua ...]\n"
                    "       luastatus -v\n"
                    "See luastatus(1) for more information.\n");
}

typedef struct {
    const char **data;
    size_t size;
    size_t capacity;
} BarlibArgs;

static inline BarlibArgs barlib_args_new(void)
{
    return (BarlibArgs) {NULL, 0, 0};
}

static inline void barlib_args_push(BarlibArgs *a, const char *s)
{
    if (a->size == a->capacity) {
        a->data = ls_x2realloc(a->data, &a->capacity, sizeof(const char *));
    }
    a->data[a->size++] = s;
}

static inline void barlib_args_free(BarlibArgs *a)
{
    free(a->data);
}

int main(int argc, char **argv)
{
    int ret = EXIT_FAILURE;
    char *barlib_name = NULL;
    BarlibArgs barlib_args = barlib_args_new();
    bool eflag = false;
    pthread_t *threads = NULL;
    bool barlib_inited = false;

    // Parse the arguments.

    for (int c; (c = getopt(argc, argv, "b:B:l:ev")) != -1;) {
        switch (c) {
        case 'b':
            barlib_name = optarg;
            break;
        case 'B':
            barlib_args_push(&barlib_args, optarg);
            break;
        case 'l':
            if ((loglevel = loglevel_fromstr(optarg)) == LUASTATUS_LOG_LAST) {
                fprintf(stderr, "Unknown log level name '%s'.\n", optarg);
                print_usage();
                goto cleanup;
            }
            break;
        case 'e':
            eflag = true;
            break;
        case 'v':
            fprintf(stderr, "This is luastatus %s.\n", LUASTATUS_VERSION);
            goto cleanup;
        case '?':
            print_usage();
            goto cleanup;
        default:
            LS_UNREACHABLE();
        }
    }

    if (!barlib_name) {
        fprintf(stderr, "Barlib was not specified.\n");
        print_usage();
        goto cleanup;
    }

    // Prepare.

    prepare_signals();

    // Initialize the widgets.

    widgets_init(argv + optind, argc - optind);

    TRACEF("nwidgets = %zu, widgets = %p, sizeof(Widget) = %d",
           nwidgets, (void *) widgets, (int) sizeof(Widget));

    if (!nwidgets) {
        WARNF("no widgets specified (see luastatus(1) for usage info)");
    }

    // Initialize the barlib.

    barlib_args_push(&barlib_args, NULL);
    if (!barlib_init_by_name(barlib_name, barlib_args.data)) {
        FATALF("cannot load barlib '%s'", barlib_name);
        goto cleanup;
    }
    barlib_inited = true;

    // Freeze the map.
    map.frozen = true;

    // Register barlib's function at the separate state, if we are going to use it.
    if (sepstate.L) {
        register_funcs(sepstate.L, NULL);
    }

    // Spawn a thread for each successfully initialized widget, call /barlib/'s /set_error()/ method
    // on each widget whose initialization has failed.

    threads = LS_XNEW(pthread_t, nwidgets);

    for (size_t i = 0; i < nwidgets; ++i) {
        Widget *w = &widgets[i];
        if (widget_is_stillborn(w)) {
            LOCK_B();
            set_error_unlocked(i);
            UNLOCK_B();
        } else {
            register_funcs(w->L, w);
            LS_PTH_CHECK(pthread_create(&threads[i], NULL, widget_thread, w));
        }
    }

    // Run /barlib/'s event watcher, if present.

    if (barlib.iface.event_watcher) {
        if (barlib.iface.event_watcher(&barlib.data, (LuastatusBarlibEWFuncs_v1) {
                .call_begin  = ew_call_begin,
                .call_end    = ew_call_end,
                .call_cancel = ew_call_cancel,
            }) == LUASTATUS_ERR)
        {
            FATALF("barlib's event_watcher() reported fatal error");
            fatal_error_reported();
        }
    }

    // Join the widget threads.

    DEBUGF("joining all the widget threads");
    for (size_t i = 0; i < nwidgets; ++i) {
        if (!widget_is_stillborn(&widgets[i])) {
            LS_PTH_CHECK(pthread_join(threads[i], NULL));
        }
    }

    // Either hang or exit.

    WARNF("all plugins' run() and barlib's event_watcher() have returned");
    if (eflag) {
        INFOF("-e passed, exiting");
        ret = EXIT_SUCCESS;
    } else {
        INFOF("since -e not passed, will hang now");
        while (1) {
            pause();
        }
    }

cleanup:
    // Let us please valgrind.
    barlib_args_free(&barlib_args);
    free(threads);
    widgets_destroy();
    if (barlib_inited) {
        barlib_destroy();
    }
    sepstate_maybe_destroy();
    map_destroy();
    return ret;
}
