#ifndef luastatus_include_plugin_data_h_
#define luastatus_include_plugin_data_h_

#include <lua.h>
#include <stddef.h>

#include "common.h"

#ifndef LUASTATUS_PLUGIN_SAYF_ATTRIBUTE
#   ifdef __GNUC__
#       define LUASTATUS_PLUGIN_SAYF_ATTRIBUTE __attribute__((format(printf, 3, 4)))
#   else
#       define LUASTATUS_PLUGIN_SAYF_ATTRIBUTE /*nothing*/
#   endif
#endif

typedef LUASTATUS_PLUGIN_SAYF_ATTRIBUTE void LuastatusPluginSayf_v1(
    void *userdata, int level, const char *fmt, ...);

typedef struct {
    // This widget's private data.
    void *priv;

    // A black-box user data.
    void *userdata;

    // Should be used for logging.
    LuastatusPluginSayf_v1 *sayf;

    // See DOCS/design/map_get.md.
    void ** (*map_get)(void *userdata, const char *key);
} LuastatusPluginData_v1;

typedef struct {
    lua_State *(*call_begin) (void *userdata);
    void       (*call_end)   (void *userdata);
    void       (*call_cancel)(void *userdata);
} LuastatusPluginRunFuncs_v1;

typedef struct {
    // This function should initialize a widget by assigning something to pd->priv.
    // You would typically do:
    //
    //     typedef struct {
    //         ...
    //     } Priv;
    //     ...
    //     static
    //     int
    //     init(LuastatusPluginData *pd, lua_State *L) {
    //         Priv *p = pd->priv = LS_XNEW(Priv, 1);
    //         ...
    //
    // The options table is on the top of L's stack.
    //
    // This function may push elements onto L's stack to iterate over tables, but should not modify
    // elements below the initial top, or interact with L in any other way.
    //
    // It is guaranteed that L's stack has at least 15 free slots.
    //
    // It should return:
    //
    //   LUASTATUS_RES_OK on success.
    //     In this case, L's stack should not contain any extra elements pushed onto it;
    //
    //   LUASTATUS_RES_ERR on failure.
    //     In this case, L's stack may contain extra elements pushed onto it.
    //
    int (*init)(LuastatusPluginData_v1 *pd, lua_State *L);

    // This function should register Lua functions provided by the plugin into table on top of L's
    // stack.
    //
    // It is guaranteed that L's stack has at least 15 free slots.
    //
    // May be NULL.
    void (*register_funcs)(LuastatusPluginData_v1 *pd, lua_State *L);

    // This function should run a widget. Once the plugin wants to update the widget, it should call
    // call_begin(pd->userdata), thus obtaining a lua_State* object (let's call it L). Then it
    // should push exactly one value onto L's stack and call call_end(pd->userdata).
    //
    // It is guaranteed that L's stack has at least 15 free slots.
    //
    // It should only return on an unrecoverable failure.
    //
    // It is explicitly allowed to call call_begin() and return without calling call_end(), leaving
    // arbitrary number of extra elements on L's stack.
    //
    void (*run)(LuastatusPluginData_v1 *pd, LuastatusPluginRunFuncs_v1 funcs);

    // This function should destroy a previously successfully initialized widget.
    void (*destroy)(LuastatusPluginData_v1 *pd);
} LuastatusPluginIface_v1;

#endif
