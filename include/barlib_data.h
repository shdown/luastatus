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

typedef LUASTATUS_BARLIB_SAYF_ATTRIBUTE void LuastatusBarlibSayf(void *userdata,
                                                                 int level,
                                                                 const char *fmt, ...);

typedef struct {
    void *priv;
    void *userdata;
    LuastatusBarlibSayf *sayf;
} LuastatusBarlibData;

typedef lua_State *LuastatusBarlibEWCallBegin(void *userdata, size_t widget_idx);
typedef void       LuastatusBarlibEWCallEnd(void *userdata, size_t widget_idx);

typedef struct {
    // This function must initialize a barlib by assigning something to bd->priv.
    //
    // You would typically do:
    //     typedef struct {
    //         ...
    //     } Priv;
    //     ...
    //     static
    //     LuastatusBarlibInitResult
    //     init(LuastatusBarlibData *bd, const char *const *opts, size_t nwidgets)
    //     {
    //         Priv *p = bd->priv = LS_NEW(Priv, 1);
    //         ...
    //
    // opts are options passed to luastatus with -B switches, last element is NULL.
    //
    // nwidgets is the number of widgets. It stays unchanged during the entire life cycle of a
    // barlib.
    //
    // It must return:
    //
    //     LUASTATUS_RES_ERR on failure;
    //
    //     LUASTATUS_RES_OK on success.
    //
    int (*init)(LuastatusBarlibData *bd, const char *const *opts, size_t nwidgets);

    // This function must assign Lua functions that the barlib provides to the table on the top of
    // L's stack.
    //
    // It is guaranteed that L's stack has at least 15 free slots.
    //
    // May be NULL.
    //
    void (*register_funcs)(LuastatusBarlibData *bd, lua_State *L);

    // This function must update the widget with index widget_idx. The data is on the top of L's
    // stack. The format of the data is defined by the barlib itself.
    //
    // This function is allowed to push elements onto L's stack to iterate over tables, but is not
    // allowed to modify stack elements, read elements below the initial top, or interact with L in
    // any other way.
    //
    // It is guaranteed that L's stack has at least 15 free slots.
    //
    // It must return:
    //
    //   LUASTATUS_RES_OK on success.
    //     In this case, L's stack must not contain any extra elements pushed onto it;
    //
    //  LUASTATUS_RES_NONFATAL_ERR on a non-fatal error, e.g., if the format of the data on top of
    //     L's stack is invalid. In this case, L's stack may contain extra elements pushed onto it;
    //
    //  LUASTATUS_RES_ERR on a fatal error, e.g. if the connection to the display has been lost. In
    //     this case, L's stack may contain extra elements pushed onto it.
    //
    int (*set)(LuastatusBarlibData *bd, lua_State *L, size_t widget_idx);

    // This function must update the widget with index widget_idx and set its
    // content to something that indicates an error.
    //
    // It must return:
    //
    //   LUASTATUS_RES_OK on success;
    //
    //   LUASTATUS_RES_NONFATAL_ERR on a fatal error, e.g. if the connection to the display has been
    //     lost.
    //
    int (*set_error)(LuastatusBarlibData *bd, size_t widget_idx);

    // This function must launch barlib's event watcher. Once the barlib wants to report an event
    // occurred on widget with index widget_idx, it must call call_begin(widget_idx, bd->userdata),
    // thus obtaining a lua_State* object (it is guaranteed that its stack has at least 15 free
    // slots).
    // Then it must push exactly one value onto its stack and call call_end(widget_idx,
    //                                                                      bd->userdata).
    // It must not call call_begin() and return without calling call_end().
    //
    // It must return:
    //   LUASTATUS_RES_NONFATAL_ERR if it is clear that no events will be reported anymore;
    //
    //   LUASTATUS_RES_ERR on a fatal error, e.g. if the connection to the display has been lost.
    //
    // May be NULL (and should be NULL if events are not supported).
    //
    int (*event_watcher)(LuastatusBarlibData *bd,
                         LuastatusBarlibEWCallBegin call_begin,
                         LuastatusBarlibEWCallEnd call_end);

    // This function must destroy a previously successfully initialized barlib.
    void (*destroy)(LuastatusBarlibData *bd);

    // See ../DOCS/WRITING_BARLIB_OR_PLUGIN.md.
    //
    // May be NULL.
    //
    const char *const *taints;
} LuastatusBarlibIface;

#endif
