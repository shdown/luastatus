#ifndef luastatus_include_barlib_data_h_
#define luastatus_include_barlib_data_h_

#include <lua.h>
#include <stddef.h>

#include "common.h"

#ifndef LUASTATUS_BARLIB_SAYF_ATTRIBUTE
#   ifdef __GNUC__
#       define LUASTATUS_BARLIB_SAYF_ATTRIBUTE __attribute__((format(printf, 3, 4)))
#   else
#       define LUASTATUS_BARLIB_SAYF_ATTRIBUTE /*nothing*/
#   endif
#endif

typedef LUASTATUS_BARLIB_SAYF_ATTRIBUTE void LuastatusBarlibSayf_v1(
    void *userdata, int level, const char *fmt, ...);

typedef struct {
    // This barlib's private data.
    void *priv;

    // A black-box user data.
    void *userdata;

    // Should be used for logging.
    LuastatusBarlibSayf_v1 *sayf;

    // See DOCS/design/map_get.md.
    void ** (*map_get)(void *userdata, const char *key);
} LuastatusBarlibData_v1;

typedef struct {
    lua_State *(*call_begin) (void *userdata, size_t widget_idx);
    void       (*call_end)   (void *userdata, size_t widget_idx);
    void       (*call_cancel)(void *userdata, size_t widget_idx);
} LuastatusBarlibEWFuncs_v1;

typedef struct {
    // This function should initialize a barlib by assigning something to /bd->priv/.
    // You would typically do:
    //
    //     typedef struct {
    //         ...
    //     } Priv;
    //     ...
    //     static
    //     int
    //     init(LuastatusBarlibData *bd, const char *const *opts, size_t nwidgets)
    //     {
    //         Priv *p = bd->priv = LS_XNEW(Priv, 1);
    //         ...
    //
    // /opts/ are options passed to luastatus with /-B/ switches, with a sentinel /NULL/ entry.
    //
    // /nwidgets/ is the number of widgets. It stays unchanged during the entire life cycle of a
    // barlib.
    //
    // It should return:
    //
    //     /LUASTATUS_ERR/ on failure;
    //
    //     /LUASTATUS_OK/ on success.
    //
    int (*init)(LuastatusBarlibData_v1 *bd, const char *const *opts, size_t nwidgets);

    // This function should register Lua functions provided by the barlib into the table on the top
    // of /L/'s stack.
    //
    // These functions should be thread-safe!
    //
    // It is guaranteed that /L/ stack has at least 15 free stack slots.
    //
    // May be /NULL/.
    //
    void (*register_funcs)(LuastatusBarlibData_v1 *bd, lua_State *L);

    // This function should update the widget with index /widget_idx/. The data is located on the
    // top of /L/'s stack; its format defined by the barlib itself.
    //
    // This function may push elements onto /L/'s stack (to iterate over tables), but should not
    // modify elements below the initial top, or interact with /L/ in any other way.
    //
    // It is guaranteed that /L/'s stack has at least 15 free slots.
    //
    // It must return:
    //
    //     /LUASTATUS_OK/ on success.
    //     In this case, /L/'s stack must not contain any extra elements pushed onto it;
    //
    //     /LUASTATUS_NONFATAL_ERR/ on a non-fatal error, e.g., if the format of the data on top of
    //     /L/'s stack is invalid.
    //     In this case, /L/'s stack may contain extra elements pushed onto it;
    //
    //     /LUASTATUS_ERR/ on a fatal error, e.g. if the connection to the display has been lost.
    //     In this case, /L/'s stack may contain extra elements pushed onto it.
    //
    int (*set)(LuastatusBarlibData_v1 *bd, lua_State *L, size_t widget_idx);

    // This function should update the widget with index /widget_idx/ and set its content to
    // something that indicates an error.
    //
    // It must return:
    //
    //     /LUASTATUS_OK/ on success;
    //
    //     /LUASTATUS_ERR/ on a fatal error, e.g. if the connection to the display has been lost.
    //
    int (*set_error)(LuastatusBarlibData_v1 *bd, size_t widget_idx);

    // This function should run barlib's event watcher. Once the barlib wants to report an event
    // occurred on widget with index /i/, it should call
    //     /lua_State *L = funcs.call_begin(i, bd->userdata)/,
    // thus obtaining a /lua_State */ object /L/. Then, it must either:
    // 1.  make the following call, with exactly one additional value pushed onto /L/'s stack:
    //         /funcs.call_end(i, bd->userdata)/,
    //     or
    // 2.  make the following call, with any number of additional values push onto /L/'s stack:
    //         /funcs.call_cancel(i, bd->userdata)/.
    //
    // Both /funcs.call_end/ and /funcs.call_cancel/ invalidate /L/ as a pointer.
    //
    // It is guaranteed that each /L/ object returned from /funcs.call_begin/ has at least 15 free
    // stack slots.
    //
    // It must return:
    //     /LUASTATUS_NONFATAL_ERR/ if, in some reason other than a fatal error, no events will be
    //     reported anymore;
    //
    //     /LUASTATUS_ERR/ on a fatal error, e.g. if the connection to the display has been lost.
    //
    // May be /NULL/ (and should be /NULL/ if events are not supported).
    //
    int (*event_watcher)(LuastatusBarlibData_v1 *bd, LuastatusBarlibEWFuncs_v1 funcs);

    // This function must destroy a previously successfully initialized barlib.
    void (*destroy)(LuastatusBarlibData_v1 *bd);
} LuastatusBarlibIface_v1;

#endif
