/*
 * Copyright (C) 2015-2025  luastatus developers
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

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/XKB.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBstr.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "libls/ls_alloc_utils.h"
#include "libls/ls_strarr.h"

#include "wrongly.h"
#include "somehow.h"

// If this plugin is used, the whole process gets killed if a connection to the display is lost,
// because Xlib is terrible.
//
// See:
// * https://tronche.com/gui/x/xlib/event-handling/protocol-errors/XSetIOErrorHandler.html
// * https://tronche.com/gui/x/xlib/event-handling/protocol-errors/XSetErrorHandler.html

typedef enum {
    HOW_WRONGLY,
    HOW_SOMEHOW,
} How;

typedef struct {
    char *dpyname;
    uint64_t deviceid;
    How how;
    char *somehow_bad;
    bool led;
} Priv;

static int parse_how_str(MoonVisit *mv, void *ud, const char *s, size_t ns)
{
    (void) ns;
    How *out = ud;
    if (strcmp(s, "wrongly") == 0) {
        *out = HOW_WRONGLY;
        return 1;
    }
    if (strcmp(s, "somehow") == 0) {
        *out = HOW_SOMEHOW;
        return 1;
    }
    moon_visit_err(mv, "unknown how string: '%s'", s);
    return -1;
}

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->dpyname);
    free(p->somehow_bad);
    free(p);
}

static int init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .dpyname = NULL,
        .deviceid = XkbUseCoreKbd,
        .how = HOW_WRONGLY,
        .somehow_bad = NULL,
        .led = false,
    };

    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse display
    if (moon_visit_str(&mv, -1, "display", &p->dpyname, NULL, true) < 0)
        goto mverror;

    // Parse device_id
    if (moon_visit_uint(&mv, -1, "device_id", &p->deviceid, true) < 0)
        goto mverror;

    // Parse how
    if (moon_visit_str_f(&mv, -1, "how", parse_how_str, &p->how, true) < 0)
        goto mverror;

    // Parse somehow_bad
    if (moon_visit_str(&mv, -1, "somehow_bad", &p->somehow_bad, NULL, true) < 0)
        goto mverror;

    if (!p->somehow_bad)
        p->somehow_bad = ls_xstrdup("group,inet,pc");

    // Parse led
    if (moon_visit_bool(&mv, -1, "led", &p->led, true) < 0)
        goto mverror;

    // Call 'XInitThreads()' if we are the first entity to use libx11.
    static char dummy[1];
    void **ptr = pd->map_get(pd->userdata, "flag:library_used:x11");
    if (!*ptr) {
        if (!XInitThreads()) {
            LS_FATALF(pd, "XInitThreads() failed");
            goto error;
        }
        *ptr = dummy;
    }

    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static Display *open_dpy(
        LuastatusPluginData *pd,
        char *dpyname,
        int *ext_base_ev_code)
{
    XkbIgnoreExtension(False);

    int event_out;
    int error_out;
    int reason_out;
    int major_in_out = XkbMajorVersion;
    int minor_in_out = XkbMinorVersion;

    Display *dpy = XkbOpenDisplay(
        dpyname, &event_out, &error_out, &major_in_out, &minor_in_out, &reason_out);

    if (dpy && reason_out == XkbOD_Success) {
        *ext_base_ev_code = event_out;
        return dpy;
    }

    const char *msg;
    switch (reason_out) {
    case XkbOD_BadLibraryVersion:
        msg = "bad XKB library version";
        break;
    case XkbOD_ConnectionRefused:
        msg = "can't open display, connection refused";
        break;
    case XkbOD_BadServerVersion:
        msg = "server has an incompatible extension version";
        break;
    case XkbOD_NonXkbServer:
        msg = "extension is not present in the server";
        break;
    default:
        msg = "unknown error";
        break;
    }
    LS_FATALF(pd, "XkbOpenDisplay() failed: %s", msg);
    return NULL;
}

static bool query_wrongly(LuastatusPluginData *pd, Display *dpy, LS_StringArray *groups)
{
    ls_strarr_clear(groups);

    WronglyResult res;
    if (!wrongly_fetch(dpy, &res)) {
        LS_WARNF(pd, "[wrongly] wrongly_fetch() failed");
        return false;
    }

    if (res.layout) {
        LS_DEBUGF(pd, "[wrongly] layout string: %s", res.layout);
        wrongly_parse_layout(res.layout, groups);
    } else {
        LS_DEBUGF(pd, "[wrongly] layout string is NULL");
    }

    free(res.rules);
    free(res.model);
    free(res.layout);
    free(res.options);

    return true;
}

static bool query_somehow(LuastatusPluginData *pd, Display *dpy, LS_StringArray *groups)
{
    ls_strarr_clear(groups);

    Priv *p = pd->priv;
    char *res = somehow_fetch_symbols(dpy, p->deviceid);
    if (!res) {
        LS_WARNF(pd, "[somehow] somehow_fetch_symbols() failed");
        return false;
    }

    LS_DEBUGF(pd, "[somehow] symbols string: %s", res);
    somehow_parse_symbols(res, groups, p->somehow_bad);

    free(res);

    return true;
}

static inline bool query(LuastatusPluginData *pd, Display *dpy, LS_StringArray *groups)
{
    Priv *p = pd->priv;
    switch (p->how) {
    case HOW_WRONGLY:
        return query_wrongly(pd, dpy, groups);
    case HOW_SOMEHOW:
        return query_somehow(pd, dpy, groups);
    }
    LS_MUST_BE_UNREACHABLE();
}

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;
    LS_StringArray groups = ls_strarr_new();
    Display *dpy = NULL;
    int ext_base_ev_code;

    if (!(dpy = open_dpy(pd, p->dpyname, &ext_base_ev_code)))
        goto error;

    // So, the X server maintains the bit mask of events we are interesed in; we can alter this mask
    // for XKB-related events with 'XkbSelectEvents()' and 'XkbSelectEventDetails()'.
    // Both of those functions take
    //    'unsigned long bits_to_change,
    //     unsigned long values_for_bits'
    // as the last two arguments. What it means is that, by invoking those functions, we say that we
    // only want to *change* the bits in our current event mask that are *set* in 'bits_to_change';
    // and that we want to assign them the *values* of corresponding bits in 'values_for_bits'.
    //
    // In other words, the algorithm for the server is something like this:
    //
    //     // clear bits that the client wants to be changed
    //     mask &= ~bits_to_change;
    //
    //     // set bits that the client wants to set
    //     mask |= (values_for_bits & bits_to_change);

    // These are events that any XKB client *must* listen to, unrelated to layout/LED.
    const unsigned long BOILERPLATE_EVENTS_MASK =
        XkbNewKeyboardNotifyMask |
        XkbMapNotifyMask;

    // These are events related to LED.
    const unsigned long LED_EVENTS_MASK =
        XkbIndicatorStateNotifyMask |
        XkbIndicatorMapNotifyMask;

    if (XkbSelectEvents(
                dpy,
                p->deviceid,
                /*bits_to_change=*/BOILERPLATE_EVENTS_MASK,
                /*values_for_bits=*/BOILERPLATE_EVENTS_MASK)
            == False)
    {
        LS_FATALF(pd, "XkbSelectEvents() failed (boilerplate events)");
        goto error;
    }

    if (XkbSelectEventDetails(
                dpy,
                p->deviceid,
                /*event_type=*/XkbStateNotify,
                /*bits_to_change=*/XkbAllStateComponentsMask,
                /*values_for_bits=*/XkbGroupStateMask)
            == False)
    {
        LS_FATALF(pd, "XkbSelectEventDetails() failed (layout events)");
        goto error;
    }

    if (p->led) {
        if (XkbSelectEvents(
                dpy,
                p->deviceid,
                /*bits_to_change=*/LED_EVENTS_MASK,
                /*values_for_bits=*/LED_EVENTS_MASK)
            == False)
        {
            LS_FATALF(pd, "XkbSelectEvents() failed (LED events)");
            goto error;
        }
    }

    bool requery_everything = true;
    size_t group = -1;
    unsigned led_state = -1;

    while (1) {
        // re-query everything, if needed
        if (requery_everything) {
            // re-query group names
            if (!query(pd, dpy, &groups)) {
                LS_WARNF(pd, "query() failed");
            }

            // explicitly query current group
            XkbStateRec state;
            if (XkbGetState(dpy, p->deviceid, &state) != Success) {
                LS_FATALF(pd, "XkbGetState() failed");
                goto error;
            }
            group = state.group;

            // check if group is valid and possibly re-query group names
            if (group >= ls_strarr_size(groups)) {
                LS_WARNF(pd, "group ID (%zu) is too large, requerying", group);

                if (!query(pd, dpy, &groups)) {
                    LS_WARNF(pd, "query() failed");
                }

                if (group >= ls_strarr_size(groups)) {
                    LS_WARNF(pd, "group ID is still too large");
                }
            }

            // explicitly query current state of LED indicators
            if (p->led) {
                if (XkbGetIndicatorState(dpy, p->deviceid, &led_state) != Success) {
                    LS_FATALF(pd, "XkbGetIndicatorState() failed");
                    goto error;
                }
            }
        }

        // make a call
        lua_State *L = funcs.call_begin(pd->userdata); // L: -
        lua_createtable(L, 0, 2); // L: table
        lua_pushinteger(L, group); // L: table n
        lua_setfield(L, -2, "id"); // L: table
        if (group < ls_strarr_size(groups)) {
            size_t nbuf;
            const char *buf = ls_strarr_at(groups, group, &nbuf);
            lua_pushlstring(L, buf, nbuf); // L: table name
            lua_setfield(L, -2, "name"); // L: table
        }
        if (p->led) {
            lua_pushinteger(L, led_state); // L: table n
            lua_setfield(L, -2, "led_state"); // L: table
        }
        if (requery_everything) {
            lua_pushboolean(L, true); // L: table true
            lua_setfield(L, -2, "requery"); // L: table
        }
        funcs.call_end(pd->userdata);

        // wait for next event
        XEvent event = {0};
        XNextEvent(dpy, &event);

        // interpret the event
        requery_everything = false;
        if (event.type == ext_base_ev_code) {

            XkbAnyEvent ev1;
            memcpy(&ev1, &event, sizeof(ev1));

            switch (ev1.xkb_type) {
            case XkbNewKeyboardNotify:
            case XkbMapNotify:
                requery_everything = true;
                break;
            case XkbStateNotify:
                {
                    XkbStateNotifyEvent ev2;
                    memcpy(&ev2, &event, sizeof(ev2));
                    group = ev2.group;
                }
                break;
            case XkbIndicatorStateNotify:
            case XkbIndicatorMapNotify:
                {
                    XkbIndicatorNotifyEvent ev2;
                    memcpy(&ev2, &event, sizeof(ev2));
                    led_state = ev2.state;
                }
                break;
            default:
                LS_WARNF(pd, "got XKB event of unknown xkb_type %d", ev1.xkb_type);
            }
        } else {
            LS_WARNF(pd, "got X event of unknown type %d", event.type);
        }
    }

error:
    if (dpy)
        XCloseDisplay(dpy);
    ls_strarr_destroy(groups);
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
