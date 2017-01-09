#ifndef widget_h_
#define widget_h_

#include <stdbool.h>
#include <lua.h>
#include <pthread.h>
#include "include/plugin_data.h"
#include "plugin.h"

typedef enum {
    WIDGET_STATE_DUMMY,
    WIDGET_STATE_LOADED,
    WIDGET_STATE_INITED,
} WidgetState;

typedef struct {
    // DUMMY: undefined
    // LOADED, INITED: a loaded Plugin
    Plugin plugin;

    // DUMMY, LOADED: undefined
    // INITED: plugin's data
    LuastatusPluginData data;

    // DUMMY: an empty Lua state
    // LOADED, INITED: a Lua state initialized from widget's file
    lua_State *L;

    // DUMMY, LOADED, INITED: an initialized pthread_mutex_t
    pthread_mutex_t lua_mtx;

    // DUMMY: undefined
    // LOADED, INITED: a reference to the 'cb' function
    int lua_ref_cb;

    // DUMMY: LUA_REFNIL
    // LOADED, INITED: LUA_REFNIL or a reference to the 'event' function
    int lua_ref_event;

    // DUMMY: undefined
    // LOADED, INITED: path to widget's file
    char *filename;

    WidgetState state;
} Widget;

bool
widget_load(Widget *w, const char *filename);

void
widget_load_dummy(Widget *w);

bool
widget_init(Widget *w, LuastatusPluginData data);

void
widget_uninit(Widget *w);

void
widget_unload(Widget *w);

#endif
