#include "include/barlib.h"
#include "include/barlib_logf_macros.h"

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <lua.h>
#include <lauxlib.h>

#include "libls/alloc_utils.h"
#include "libls/cstring_utils.h"
#include "libls/string.h"
#include "libls/vector.h"

typedef struct {
    size_t nwidgets;
    LSString *bufs;
    LSString glued;
    char *sep;
    xcb_connection_t *conn;
    xcb_window_t root;
} Priv;

void
priv_destroy(Priv *p)
{
    for (size_t i = 0; i < p->nwidgets; ++i) {
        LS_VECTOR_FREE(p->bufs[i]);
    }
    free(p->bufs);
    LS_VECTOR_FREE(p->glued);
    free(p->sep);
    if (p->conn) {
        xcb_disconnect(p->conn);
    }
}

bool
redraw(LuastatusBarlibData *bd)
{
    Priv *p = bd->priv;

    LSString *glued = &p->glued;
    size_t n = p->nwidgets;
    LSString *bufs = p->bufs;
    const char *sep = p->sep;

    LS_VECTOR_CLEAR(*glued);
    for (size_t i = 0; i < n; ++i) {
        if (bufs[i].size) {
            if (glued->size) {
                ls_string_append_s(glued, sep);
            }
            ls_string_append_b(glued, bufs[i].data, bufs[i].size);
        }
    }
    xcb_generic_error_t *err = xcb_request_check(
        p->conn,
        xcb_change_property_checked(p->conn, XCB_PROP_MODE_REPLACE, p->root, XCB_ATOM_WM_NAME,
            XCB_ATOM_STRING, /*apparently, UTF-8*/ 8, glued->size, glued->data));
    if (err) {
        LUASTATUS_FATALF(bd, "XCB error %d occured", err->error_code);
        free(err);
        return false;
    }
    return true;
}

LuastatusBarlibInitResult
init(LuastatusBarlibData *bd, const char *const *opts, size_t nwidgets)
{
    Priv *p = bd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .nwidgets = nwidgets,
        .bufs = LS_XNEW(LSString, nwidgets),
        .glued = LS_VECTOR_NEW_RESERVE(char, 1024),
        .sep = NULL,
        .conn = NULL,
    };
    for (size_t i = 0; i < nwidgets; ++i) {
        LS_VECTOR_INIT_RESERVE(p->bufs[i], 512);
    }
    // display=, separator= may be specified multiple times
    const char *dpyname = NULL;
    const char *sep = NULL;
    for (const char *const *s = opts; *s; ++s) {
        const char *v;
        if ((v = ls_strfollow(*s, "display="))) {
            dpyname = v;
        } else if ((v = ls_strfollow(*s, "separator="))) {
            sep = v;
        } else {
            LUASTATUS_FATALF(bd, "unknown option '%s'", *s);
            goto error;
        }
    }
    p->sep = ls_xstrdup(sep ? sep : " | ");

    int screenp;
    p->conn = xcb_connect(dpyname, &screenp);
    int r = xcb_connection_has_error(p->conn);
    if (r != 0) {
        LUASTATUS_FATALF(bd, "can't connect to display: XCB error %d", r);
        goto error;
    }
    const xcb_setup_t *setup = xcb_get_setup(p->conn);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    for (int i = 0; i < screenp; ++i) {
        xcb_screen_next(&iter);
    }
    p->root = iter.data->root;
    // clear the current name
    if (!redraw(bd)) {
        goto error;
    }
    return LUASTATUS_BARLIB_INIT_RESULT_OK;

error:
    priv_destroy(p);
    free(p);
    return LUASTATUS_BARLIB_INIT_RESULT_ERR;
}

LuastatusBarlibSetResult
set(LuastatusBarlibData *bd, lua_State *L, size_t widget_idx)
{
    Priv *p = bd->priv;
    int type = lua_type(L, -1);
    if (type == LUA_TSTRING) {
        size_t ns;
        const char *s = lua_tolstring(L, -1, &ns);
        ls_string_assign_b(&p->bufs[widget_idx], s, ns);
    } else if (type == LUA_TNIL) {
        LS_VECTOR_CLEAR(p->bufs[widget_idx]);
    } else {
        LUASTATUS_ERRF(bd, "expected string or nil, found %s", luaL_typename(L, -1));
        return LUASTATUS_BARLIB_SET_RESULT_NONFATAL_ERR;
    }
    if (!redraw(bd)) {
        return LUASTATUS_BARLIB_SET_RESULT_FATAL_ERR;
    }
    return LUASTATUS_BARLIB_SET_RESULT_OK;
}

LuastatusBarlibSetErrorResult
set_error(LuastatusBarlibData *bd, size_t widget_idx)
{
    Priv *p = bd->priv;
    ls_string_assign_s(&p->bufs[widget_idx], "(Error)");
    if (!redraw(bd)) {
        return LUASTATUS_BARLIB_SET_ERROR_RESULT_FATAL_ERR;
    }
    return LUASTATUS_BARLIB_SET_ERROR_RESULT_OK;
}

void
destroy(LuastatusBarlibData *bd)
{
    priv_destroy(bd->priv);
    free(bd->priv);
}

LuastatusBarlibIface luastatus_barlib_iface = {
    .init = init,
    .set = set,
    .set_error = set_error,
    .destroy = destroy,
};
