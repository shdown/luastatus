#include "include/barlib_v1.h"
#include "include/sayf_macros.h"

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <lua.h>
#include <lauxlib.h>

#include "libls/alloc_utils.h"
#include "libls/cstring_utils.h"
#include "libls/string_.h"
#include "libls/vector.h"
#include "libls/lua_utils.h"

typedef struct {
    size_t nwidgets;

    LSString *bufs;

    // Temporary buffer for secondary buffering, to avoid unneeded redraws.
    LSString tmpbuf;

    // Buffer for the content of the widgets joined by /sep/.
    LSString joined;

    char *sep;

    xcb_connection_t *conn;

    xcb_window_t root;
} Priv;

static
void
destroy(LuastatusBarlibData *bd)
{
    Priv *p = bd->priv;
    for (size_t i = 0; i < p->nwidgets; ++i) {
        LS_VECTOR_FREE(p->bufs[i]);
    }
    free(p->bufs);
    LS_VECTOR_FREE(p->tmpbuf);
    LS_VECTOR_FREE(p->joined);
    free(p->sep);
    if (p->conn) {
        xcb_disconnect(p->conn);
    }
    free(p);
}

static
bool
redraw(LuastatusBarlibData *bd)
{
    Priv *p = bd->priv;

    LSString *joined = &p->joined;
    size_t n = p->nwidgets;
    LSString *bufs = p->bufs;
    const char *sep = p->sep;

    LS_VECTOR_CLEAR(*joined);
    for (size_t i = 0; i < n; ++i) {
        if (bufs[i].size) {
            if (joined->size) {
                ls_string_append_s(joined, sep);
            }
            ls_string_append_b(joined, bufs[i].data, bufs[i].size);
        }
    }

    xcb_generic_error_t *err = xcb_request_check(
        p->conn,
        xcb_change_property_checked(
            p->conn, XCB_PROP_MODE_REPLACE, p->root, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
            joined->size, joined->data));
    if (err) {
        LS_FATALF(bd, "XCB error %d occured", err->error_code);
        free(err);
        return false;
    }
    return true;
}

static
int
init(LuastatusBarlibData *bd, const char *const *opts, size_t nwidgets)
{
    Priv *p = bd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .nwidgets = nwidgets,
        .bufs = LS_XNEW(LSString, nwidgets),
        .tmpbuf = LS_VECTOR_NEW(),
        .joined = LS_VECTOR_NEW_RESERVE(char, 1024),
        .sep = NULL,
        .conn = NULL,
    };
    for (size_t i = 0; i < nwidgets; ++i) {
        LS_VECTOR_INIT_RESERVE(p->bufs[i], 512);
    }
    // All the options may be passed multiple times!
    const char *dpyname = NULL;
    const char *sep = NULL;
    for (const char *const *s = opts; *s; ++s) {
        const char *v;
        if ((v = ls_strfollow(*s, "display="))) {
            dpyname = v;
        } else if ((v = ls_strfollow(*s, "separator="))) {
            sep = v;
        } else {
            LS_FATALF(bd, "unknown option '%s'", *s);
            goto error;
        }
    }
    p->sep = ls_xstrdup(sep ? sep : " | ");

    // Initialize /p->conn/ and /p->root/.
    int screenp;
    p->conn = xcb_connect(dpyname, &screenp);
    int r = xcb_connection_has_error(p->conn);
    if (r != 0) {
        LS_FATALF(bd, "can't connect to display: XCB error %d", r);
        goto error;
    }
    const xcb_setup_t *setup = xcb_get_setup(p->conn);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    for (int i = 0; i < screenp; ++i) {
        xcb_screen_next(&iter);
    }
    p->root = iter.data->root;

    // Clear the current name.
    if (!redraw(bd)) {
        goto error;
    }

    return LUASTATUS_OK;

error:
    destroy(bd);
    return LUASTATUS_ERR;
}

static
int
set(LuastatusBarlibData *bd, lua_State *L, size_t widget_idx)
{
    Priv *p = bd->priv;
    LSString *buf = &p->tmpbuf;
    LS_VECTOR_CLEAR(*buf);

    switch (lua_type(L, -1)) {
    case LUA_TSTRING:
        {
            size_t ns;
            const char *s = lua_tolstring(L, -1, &ns);
            ls_string_assign_b(buf, s, ns);
        }
        break;
    case LUA_TNIL:
        break;
    case LUA_TTABLE:
        {
            const char *sep = p->sep;

            LS_LUA_TRAVERSE(L, -1) {
                if (!lua_isnumber(L, LS_LUA_KEY)) {
                    LS_ERRF(bd, "table key: expected number, found %s",
                        luaL_typename(L, LS_LUA_KEY));
                    goto invalid_data;
                }
                if (!lua_isstring(L, LS_LUA_VALUE)) {
                    LS_ERRF(bd, "table value: expected string, found %s",
                        luaL_typename(L, LS_LUA_VALUE));
                    goto invalid_data;
                }
                size_t ns;
                const char *s = lua_tolstring(L, LS_LUA_VALUE, &ns);
                if (buf->size && ns) {
                    ls_string_append_s(buf, sep);
                }
                ls_string_append_b(buf, s, ns);
            }
        }
        break;
    default:
        LS_ERRF(bd, "expected table, string or nil, found %s", luaL_typename(L, -1));
        goto invalid_data;
    }

    if (!ls_string_eq(*buf, p->bufs[widget_idx])) {
        ls_string_swap(buf, &p->bufs[widget_idx]);
        if (!redraw(bd)) {
            return LUASTATUS_ERR;
        }
    }
    return LUASTATUS_OK;

invalid_data:
    LS_VECTOR_CLEAR(p->bufs[widget_idx]);
    return LUASTATUS_NONFATAL_ERR;
}

static
int
set_error(LuastatusBarlibData *bd, size_t widget_idx)
{
    Priv *p = bd->priv;
    ls_string_assign_s(&p->bufs[widget_idx], "(Error)");
    if (!redraw(bd)) {
        return LUASTATUS_ERR;
    }
    return LUASTATUS_OK;
}

LuastatusBarlibIface luastatus_barlib_iface_v1 = {
    .init = init,
    .set = set,
    .set_error = set_error,
    .destroy = destroy,
};
