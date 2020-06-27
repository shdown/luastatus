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

// Returns zero on success, non-zero XCB error code on failure. In either case, /*out_conn/ is
// written to, and should be closed with /xcb_disconnect()/.
static
int
connect(const char *dpyname, xcb_connection_t **out_conn, xcb_window_t *out_root)
{
    int screenp;
    *out_conn = xcb_connect(dpyname, &screenp);
    const int r = xcb_connection_has_error(*out_conn);
    if (r != 0) {
        return r;
    }

    const xcb_setup_t *setup = xcb_get_setup(*out_conn);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    for (int i = 0; i < screenp; ++i) {
        xcb_screen_next(&iter);
    }
    *out_root = iter.data->root;
    return 0;
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
            p->conn,
            XCB_PROP_MODE_REPLACE,
            p->root,
            XCB_ATOM_WM_NAME,
            XCB_ATOM_STRING,
            8,
            joined->size,
            joined->data
        )
    );
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

    const int r = connect(dpyname, &p->conn, &p->root);
    if (r != 0) {
        LS_FATALF(bd, "can't connect to display: XCB error %d", r);
        goto error;
    }

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

    // L: ? data

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

            lua_pushnil(L); // L: ? data nil
            while (lua_next(L, -2)) {
                // L: ? data key value
                if (!lua_isstring(L, -1)) {
                    LS_ERRF(bd, "table value: expected string, found %s", luaL_typename(L, -1));
                    goto invalid_data;
                }
                size_t ns;
                const char *s = lua_tolstring(L, -1, &ns);
                if (buf->size && ns) {
                    ls_string_append_s(buf, sep);
                }
                ls_string_append_b(buf, s, ns);

                lua_pop(L, 1); // L: ? data key
            }
            // L: ? data
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
