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

#include <lua.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_ewmh.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "libls/ls_alloc_utils.h"
#include "libls/ls_tls_ebuf.h"
#include "libls/ls_io_utils.h"
#include "libls/ls_time_utils.h"

// some parts of this file (including the name) are proudly stolen from
// xtitle (https://github.com/baskerville/xtitle).

typedef struct {
    char *dpyname;
    bool visible;
    bool extended_fmt;
} Priv;

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->dpyname);
    free(p);
}

static int init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .dpyname = NULL,
        .visible = false,
        .extended_fmt = false,
    };
    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse display
    if (moon_visit_str(&mv, -1, "display", &p->dpyname, NULL, true) < 0)
        goto mverror;

    // Parse visible
    if (moon_visit_bool(&mv, -1, "visible", &p->visible, true) < 0)
        goto mverror;

    // Parse extended_fmt
    if (moon_visit_bool(&mv, -1, "extended_fmt", &p->extended_fmt, true) < 0)
        goto mverror;

    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
//error:
    destroy(pd);
    return LUASTATUS_ERR;
}

typedef struct {
    xcb_connection_t *conn;
    xcb_ewmh_connection_t *ewmh;
    bool ewmh_inited;
    xcb_window_t root;
    int screenp;
    bool visible;
    bool extended_fmt;
} Data;

static inline void do_watch(Data *d, xcb_window_t win, bool state)
{
    if (win == XCB_NONE)
        return;
    uint32_t value = state ? XCB_EVENT_MASK_PROPERTY_CHANGE : XCB_EVENT_MASK_NO_EVENT;
    xcb_change_window_attributes(d->conn, win, XCB_CW_EVENT_MASK, &value);
}

static inline bool get_active_window(Data *d, xcb_window_t *win)
{
    return
        xcb_ewmh_get_active_window_reply(
            d->ewmh,
            xcb_ewmh_get_active_window(d->ewmh, d->screenp),
            win,
            NULL
        ) == 1;
}

static bool push_window_title(Data *d, lua_State *L, xcb_window_t win)
{
    if (win == XCB_NONE)
        return false;

    xcb_ewmh_get_utf8_strings_reply_t ewmh_txt_prop;
    xcb_icccm_get_text_property_reply_t icccm_txt_prop;
    ewmh_txt_prop.strings = NULL;
    icccm_txt_prop.name = NULL;

    if (d->visible &&
        xcb_ewmh_get_wm_visible_name_reply(
            d->ewmh,
            xcb_ewmh_get_wm_visible_name(d->ewmh, win),
            &ewmh_txt_prop,
            NULL
        ) == 1 &&
        ewmh_txt_prop.strings)
    {
        lua_pushlstring(L, ewmh_txt_prop.strings, ewmh_txt_prop.strings_len);
        xcb_ewmh_get_utf8_strings_reply_wipe(&ewmh_txt_prop);
        return true;
    }

    if (xcb_ewmh_get_wm_name_reply(
            d->ewmh,
            xcb_ewmh_get_wm_name(d->ewmh, win),
            &ewmh_txt_prop,
            NULL
        ) == 1 &&
        ewmh_txt_prop.strings)
    {
        lua_pushlstring(L, ewmh_txt_prop.strings, ewmh_txt_prop.strings_len);
        xcb_ewmh_get_utf8_strings_reply_wipe(&ewmh_txt_prop);
        return true;
    }

    if (xcb_icccm_get_wm_name_reply(
            d->conn,
            xcb_icccm_get_wm_name(d->conn, win),
            &icccm_txt_prop,
            NULL
        ) == 1 &&
        icccm_txt_prop.name)
    {
        lua_pushlstring(L, icccm_txt_prop.name, icccm_txt_prop.name_len);
        xcb_icccm_get_text_property_reply_wipe(&icccm_txt_prop);
        return true;
    }

    return false;
}

static inline void push_window_title_or_nil(Data *d, lua_State *L, xcb_window_t win)
{
    if (!push_window_title(d, L, win)) {
        lua_pushnil(L);
    }
}

static bool push_window_class_and_instance(Data *d, lua_State *L, xcb_window_t win)
{
    if (win == XCB_NONE) {
        return false;
    }

    xcb_get_property_cookie_t cookie =
        xcb_icccm_get_wm_class(d->conn, win);

    xcb_icccm_get_wm_class_reply_t prop;
    if (!xcb_icccm_get_wm_class_reply(d->conn, cookie, &prop, NULL)) {
        return false;
    }

    const char *class_name = prop.class_name;
    if (!class_name) {
        class_name = "";
    }
    lua_pushstring(L, class_name); // L: class

    const char *instance_name = prop.instance_name;
    if (!instance_name) {
        instance_name = "";
    }
    lua_pushstring(L, instance_name); // L: instance

    xcb_icccm_get_wm_class_reply_wipe(&prop);

    return true;
}

static void push_arg(Data *d, lua_State *L, xcb_window_t win)
{
    if (!d->extended_fmt) {
        push_window_title_or_nil(d, L, win); // L: title
        return;
    }

    lua_createtable(L, 0, 3); // L: table

    push_window_title_or_nil(d, L, win); // L: table title
    lua_setfield(L, -2, "title"); // L: table

    if (!push_window_class_and_instance(d, L, win)) {
        lua_pushnil(L); // L: table nil
        lua_pushnil(L); // L: table nil nil
    }
    // L: table class instance
    lua_setfield(L, -3, "instance"); // L: table class
    lua_setfield(L, -2, "class"); // L: table
}

// updates /*win/ and /*last_win/ if the active window was changed
static bool title_changed(
        Data *d,
        xcb_generic_event_t *evt,
        xcb_window_t *win,
        xcb_window_t *last_win)
{
    if (XCB_EVENT_RESPONSE_TYPE(evt) != XCB_PROPERTY_NOTIFY)
        return false;

    xcb_property_notify_event_t *pne = (xcb_property_notify_event_t *) evt;

    if (pne->atom == d->ewmh->_NET_ACTIVE_WINDOW) {
        do_watch(d, *last_win, false);
        if (get_active_window(d, win)) {
            do_watch(d, *win, true);
            *last_win = *win;
        } else {
            *win = *last_win = XCB_NONE;
        }
        return true;
    }

    if (*win != XCB_NONE && pne->window == *win &&
        ((d->visible && pne->atom == d->ewmh->_NET_WM_VISIBLE_NAME) ||
         pne->atom == d->ewmh->_NET_WM_NAME ||
         pne->atom == XCB_ATOM_WM_NAME))
    {
        return true;
    }

    return false;
}

static xcb_screen_iterator_t find_nth_screen(xcb_connection_t *conn, int n)
{
    const xcb_setup_t *setup = xcb_get_setup(conn);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    for (int i = 0; i < n; ++i)
        xcb_screen_next(&iter);
    return iter;
}

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    Data d = {
        .conn = NULL,
        .ewmh = LS_XNEW(xcb_ewmh_connection_t, 1),
        .ewmh_inited = false,
        .visible = p->visible,
        .extended_fmt = p->extended_fmt,
    };
    int err;

    // connect
    d.conn = xcb_connect(p->dpyname, &d.screenp);
    if ((err = xcb_connection_has_error(d.conn))) {
        LS_FATALF(pd, "xcb_connect: XCB error %d", err);
        // /xcb_disconnect/ should be called even if /xcb_connection_has_error/ returned non-zero,
        // so we should not set /d.conn/ to /NULL/ here.
        goto error;
    }

    // iterate over screens to find our root window
    d.root = find_nth_screen(d.conn, d.screenp).data->root;

    // initialize ewmh
    if (
        xcb_ewmh_init_atoms_replies(
            d.ewmh,
            xcb_ewmh_init_atoms(d.conn, d.ewmh),
            NULL)
        == 0)
    {
        LS_FATALF(pd, "xcb_ewmh_init_atoms_replies() failed");
        goto error;
    }
    d.ewmh_inited = true;

    xcb_window_t win = XCB_NONE;
    xcb_window_t last_win = XCB_NONE;

    // get initial active window; make a call on success.
    if (get_active_window(&d, &win)) {
        push_arg(&d, funcs.call_begin(pd->userdata), win);
        funcs.call_end(pd->userdata);
    }

    // set up initial watchers
    do_watch(&d, d.root, true);
    do_watch(&d, win, true);

    // poll for changes

    int fd = xcb_get_file_descriptor(d.conn);
    xcb_flush(d.conn);
    while (1) {
        int nfds = ls_wait_input_on_fd(fd, LS_TD_FOREVER);
        if (nfds < 0) {
            LS_FATALF(pd, "ls_wait_input_on_fd: %s", ls_tls_strerror(errno));
            goto error;
        } else if (nfds > 0) {
            xcb_generic_event_t *evt;
            while ((evt = xcb_poll_for_event(d.conn))) {
                if (title_changed(&d, evt, &win, &last_win)) {
                    push_arg(&d, funcs.call_begin(pd->userdata), win);
                    funcs.call_end(pd->userdata);
                }
                free(evt);
            }
            if ((err = xcb_connection_has_error(d.conn))) {
                LS_FATALF(pd, "xcb_poll_for_event: XCB error %d", err);
                goto error;
            }
        }
    }

error:
    if (d.ewmh_inited) {
       xcb_ewmh_connection_wipe(d.ewmh);
    }
    if (d.conn) {
        xcb_disconnect(d.conn);
    }
    free(d.ewmh);
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
