#ifndef luastatus_include_plugin_data_h_
#define luastatus_include_plugin_data_h_

#include <lua.h>
#include <stddef.h>

#include "loglevel.h"

#ifndef LUASTATUS_PLUGIN_LOGF_ATTRIBUTE
#   ifdef __GNUC__
#       define LUASTATUS_PLUGIN_LOGF_ATTRIBUTE __attribute__((format(printf, 3, 4)))
#   else
#       define LUASTATUS_PLUGIN_LOGF_ATTRIBUTE /*nothing*/
#   endif
#endif

typedef LUASTATUS_PLUGIN_LOGF_ATTRIBUTE void LuastatusPluginLogf(void *userdata,
                                                                 LuastatusLogLevel level,
                                                                 const char *fmt, ...);

typedef struct {
    void *priv;
    void *userdata;
    LuastatusPluginLogf *logf;
} LuastatusPluginData;

typedef lua_State *LuastatusPluginCallBegin(void *userdata);
typedef void       LuastatusPluginCallEnd(void *userdata);

typedef enum {
    LUASTATUS_PLUGIN_INIT_RESULT_OK,
    LUASTATUS_PLUGIN_INIT_RESULT_ERR,
} LuastatusPluginInitResult;

typedef struct {
    // This function must initialize a widget by assigning something to pd->priv.
    // You would typically do:
    //
    //     typedef struct {
    //         ...
    //     } Priv;
    //     ...
    //     static
    //     LuastatusPluginInitResult
    //     init(LuastatusPluginData *pd, lua_State *L) {
    //         Priv *p = pd->priv = LS_NEW(Priv, 1);
    //         ...
    //
    // The options table is on the top of L's stack.
    //
    // This function is allowed to push elements onto L's stack to iterate over tables, but is not
    // allowed to modify stack elements, read elements below the initial top, or interact with L in
    // any other way.
    //
    // It is guaranteed that L's stack has at least 15 free slots.
    //
    // It must return:
    //
    //   LUASTATUS_PLUGIN_INIT_RESULT_OK on success.
    //     In this case, L's stack must not contain any extra elements pushed onto it;
    //
    //   LUASTATUS_PLUGIN_INIT_RESULT_ERR on failure.
    //     In this case, L's stack may contain extra elements pushed onto it.
    //
    LuastatusPluginInitResult (*init)(LuastatusPluginData *pd, lua_State *L);

    // This function must assign Lua functions that the plugin provides to the table on the top of
    // L's stack.
    //
    // It is guaranteed that L's stack has at least 15 free slots.
    //
    // May be NULL.
    void (*register_funcs)(LuastatusPluginData *pd, lua_State *L);

    // This function must launch a widget. Once the plugin wants to update the widget, it must
    // call call_begin(pd->userdata), thus obtaining a lua_State* object (it is guaranteed that its
    // stack has at least 15 free slots).
    // Then it must push exactly one value onto its stack and call call_end(pd->userdata).
    //
    // It must not call_begin() and return without calling call_end().
    //
    // It must only return on an unrecoverable failure.
    //
    void (*run)(LuastatusPluginData *pd, LuastatusPluginCallBegin call_begin,
                LuastatusPluginCallEnd call_end);

    // This function must destroy a previously successfully initialized widget.
    void (*destroy)(LuastatusPluginData *pd);

    // See ../WRITING_BARLIB_OR_PLUGIN.md.
    //
    // May be NULL.
    const char *const *taints;
} LuastatusPluginIface;

#endif
