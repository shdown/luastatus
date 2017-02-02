#include "widget.h"

#include <stdlib.h>
#include <stdbool.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <assert.h>
#include "include/loglevel.h"
#include "libls/alloc_utils.h"
#include "libls/lua_utils.h"
#include "libls/compdep.h"
#include "pth_check.h"
#include "log.h"
#include "check_lua_call.h"
#include "lua_libs.h"
#include "load_by_name.h"
#include "plugin.h"

bool
widget_load(Widget *w, const char *filename)
{
    lua_State *L = w->L = luaL_newstate();
    bool plugin_loaded = false;

    if (!L) {
        ls_oom();
    }
    luaL_openlibs(L);
    // L: -
    if (!check_lua_call(L, luaL_loadfile(L, filename))) {
        goto error;
    }
    // L: chunk
    lualibs_inject(L); // L: chunk
    if (!check_lua_call(L, lua_pcall(L, 0, 0, 0))) {
        goto error;
    }
    // L: -
    ls_lua_pushglobaltable(L); // L: _G
    ls_lua_rawgetf(L, "widget"); // L: _G widget
    if (!lua_istable(L, -1)) {
        internal_logf(LUASTATUS_ERR, "widget: expected table, found %s", luaL_typename(L, -1));
        goto error;
    }
    ls_lua_rawgetf(L, "plugin"); // L: _G widget plugin
    if (!lua_isstring(L, -1)) {
        internal_logf(LUASTATUS_ERR, "widget.plugin: expected string, found %s",
                      luaL_typename(L, -1));
        goto error;
    }
    if (!load_plugin_by_name(&w->plugin, lua_tostring(L, -1))) {
        internal_logf(LUASTATUS_ERR, "can't load plugin '%s'", lua_tostring(L, -1));
        goto error;
    }
    plugin_loaded = true;
    lua_pop(L, 1); // L: _G widget

    ls_lua_rawgetf(L, "cb"); // L: _G widget cb
    if (!lua_isfunction(L, -1)) {
        internal_logf(LUASTATUS_ERR, "widget.cb: expected function, found %s",
                      luaL_typename(L, -1));
        goto error;
    }
    w->lua_ref_cb = luaL_ref(L, LUA_REGISTRYINDEX); // L: _G widget
    ls_lua_rawgetf(L, "event"); // L: _G widget event
    if (!lua_isfunction(L, -1) && !lua_isnil(L, -1)) {
        internal_logf(LUASTATUS_ERR, "widget.event: expected function or nil, found %s",
                      luaL_typename(L, -1));
        goto error;
    }
    w->lua_ref_event = luaL_ref(L, LUA_REGISTRYINDEX); // L: _G widget
    lua_pop(L, 2); // L: -

    PTH_CHECK(pthread_mutex_init(&w->lua_mtx, NULL));
    w->filename = ls_xstrdup(filename);
    w->state = WIDGET_STATE_LOADED;
    return true;

error:
    lua_close(L);
    if (plugin_loaded) {
        plugin_unload(&w->plugin);
    }
    return false;
}

void
widget_load_dummy(Widget *w)
{
    if (!(w->L = luaL_newstate())) {
        ls_oom();
    }
    PTH_CHECK(pthread_mutex_init(&w->lua_mtx, NULL));
    w->lua_ref_event = LUA_REFNIL;
    w->state = WIDGET_STATE_DUMMY;
}

bool
widget_init(Widget *w, LuastatusPluginData data)
{
    assert(w->state == WIDGET_STATE_LOADED);

    w->data = data;
    lua_State *L = w->L;
    int init_top = lua_gettop(L);
    // L: -
    ls_lua_pushglobaltable(L); // L: _G
    ls_lua_rawgetf(L, "widget"); // L: _G widget
    assert(lua_istable(L, -1));
    ls_lua_rawgetf(L, "opts"); // L: _G widget opts
    if (!lua_istable(L, -1)) {
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1); // L: _G widget
            lua_newtable(L); // L: _G widget table
        } else {
            internal_logf(LUASTATUS_ERR, "widget.opts: expected table or nil, found %s",
                          luaL_typename(L, -1));
            goto done;
        }
    }
    switch (w->plugin.iface.init(&w->data, w->L)) {
    case LUASTATUS_PLUGIN_INIT_RESULT_OK:
        w->state = WIDGET_STATE_INITED;
        goto done;
    case LUASTATUS_PLUGIN_INIT_RESULT_ERR:
        internal_logf(LUASTATUS_ERR, "plugin's (%s) init() failed", w->plugin.name);
        goto done;
    }
    LS_UNREACHABLE();

done:
    lua_settop(L, init_top); // L: -
    return w->state == WIDGET_STATE_INITED;
}

void
widget_uninit(Widget *w)
{
    assert(w->state == WIDGET_STATE_INITED);

    w->plugin.iface.destroy(&w->data);
    w->state = WIDGET_STATE_LOADED;
}

void
widget_unload(Widget *w)
{
    switch (w->state) {
    case WIDGET_STATE_INITED:
        widget_uninit(w);
        /* fallthru */
    case WIDGET_STATE_LOADED:
        plugin_unload(&w->plugin);
        free(w->filename);
        /* fallthru */
    case WIDGET_STATE_DUMMY:
        lua_close(w->L);
        PTH_CHECK(pthread_mutex_destroy(&w->lua_mtx));
        return;
    }
    LS_UNREACHABLE();
}
