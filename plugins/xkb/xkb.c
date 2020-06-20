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

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/XKB.h>
#include <X11/XKBlib.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <lua.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"
#include "include/plugin_utils.h"

#include "libls/alloc_utils.h"
#include "libls/compdep.h"
#include "libls/strarr.h"
#include "libls/algo.h"

#include "rules_names.h"

// If this plugin is used, the whole process gets killed if a connection to the display is lost,
// because Xlib is terrible.
//
// See:
// * https://tronche.com/gui/x/xlib/event-handling/protocol-errors/XSetIOErrorHandler.html
// * https://tronche.com/gui/x/xlib/event-handling/protocol-errors/XSetErrorHandler.html

typedef struct {
    char *dpyname;
    unsigned deviceid;
    bool led;
} Priv;

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->dpyname);
    free(p);
}

static
int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .dpyname = NULL,
        .deviceid = XkbUseCoreKbd,
        .led = false,
    };

    PU_MAYBE_VISIT_STR_FIELD(-1, "display", "'display'", s,
        p->dpyname = ls_xstrdup(s);
    );

    PU_MAYBE_VISIT_NUM_FIELD(-1, "device_id", "'device_id'", n,
        if (!ls_is_between_d(n, 0, UINT_MAX)) {
            LS_FATALF(pd, "'device_id' is invalid");
            goto error;
        }
        p->deviceid = n;
    );

    PU_MAYBE_VISIT_BOOL_FIELD(-1, "led", "'led'", b,
        p->led = b;
    );

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

error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static
Display *
open_dpy(LuastatusPluginData *pd, char *dpyname, int *ext_base_ev_code)
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

static
bool
query_groups(Display *dpy, LSStringArray *groups)
{
    RulesNames rn;
    if (!rules_names_load(dpy, &rn)) {
        return false;
    }

    ls_strarr_clear(groups);
    if (rn.layout) {
        // split /rn.layout/ by non-parenthesized commas
        size_t balance = 0;
        size_t prev = 0;
        const size_t nlayout = strlen(rn.layout);
        for (size_t i = 0; i < nlayout; ++i) {
            switch (rn.layout[i]) {
            case '(': ++balance; break;
            case ')': --balance; break;
            case ',':
                if (balance == 0) {
                    ls_strarr_append(groups, rn.layout + prev, i - prev);
                    prev = i + 1;
                }
                break;
            }
        }
        ls_strarr_append(groups, rn.layout + prev, nlayout - prev);
    }

    rules_names_destroy(&rn);
    return true;
}

static
void
run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;
    LSStringArray groups = ls_strarr_new();
    Display *dpy = NULL;
    int ext_base_ev_code;

    if (!(dpy = open_dpy(pd, p->dpyname, &ext_base_ev_code))) {
        goto error;
    }

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
        XkbIndicatorStateNotifyMask
        | XkbIndicatorMapNotifyMask;

    if (XkbSelectEvents(
                dpy,
                p->deviceid,
                /*bits_to_change=*/BOILERPLATE_EVENTS_MASK,
                /*values_for_bits=*/BOILERPLATE_EVENTS_MASK)
            == False)
    {
        LS_FATALF(pd, "XkbSelectEvents() failed [boilerplate events]");
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
        LS_FATALF(pd, "XkbSelectEventDetails() failed [layout events]");
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
            LS_FATALF(pd, "XkbSelectEvents() failed [LED events]");
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
            if (!query_groups(dpy, &groups)) {
                LS_FATALF(pd, "query_groups() failed");
                goto error;
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

                if (!query_groups(dpy, &groups)) {
                    LS_FATALF(pd, "query_groups() failed");
                    goto error;
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
            lua_pushlstring(L, buf, nbuf); // L: table group
            lua_setfield(L, -2, "name"); // L: table
        }
        if (p->led) {
            lua_pushinteger(L, led_state); // L: table n
            lua_setfield(L, -2, "led_state"); // L: table
        }
        funcs.call_end(pd->userdata);

        // wait for next event
        XEvent event = {0};
        // Apparently, the meaning of the return value of 'XNextEvent()' is classified information.
        // Whatever, guys - you don't want me to check the return value, I don't do it.
        XNextEvent(dpy, &event);

        //
        // So, the authors of the X11 event handling thing surely wanted X11 clients to perform
        // casts that break the strict aliasing rule, as defined in the C standard: there are X
        // server "extensions" that define their own event structures (e.g. XKB), but 'XEvent' is
        // defined as union of everything that the X server may send *without* extensions.
        //
        // So, technically, '(XkbAnyEvent *) &event', where 'event' has type of 'XEvent', is
        // undefined behaviour. One known trick to mitigate that is to perform a memcpy:
        //     XkbAnyEvent anyev;
        //     memcpy(&anyev, &event, sizeof(anyev));
        // The overhead of copying is acceptable in our case, but not in general.
        //
        // Curiously, the Berkeley sockets API has the same problem; if 'sa' has type of, e.g.,
        // 'struct sockaddr_in', then both
        //     connect(fd, (struct sockaddr *) &sa, sizeof(sa))
        // and
        //     bind(fd, (struct sockaddr *) &sa, sizeof(sa))
        // are undefined behaviour, unless some compiler-dependent hacks are used. For compilers
        // that are GNU C-compatible, such a hack is transparent unions, and they are in fact used
        // in glibc's <sys/socket.h>:
        //
        //     typedef union { __SOCKADDR_ALLTYPES
        //                   } __SOCKADDR_ARG __attribute__ ((__transparent_union__));
        //
        // Ugh. The C standard is a terrible mess. X.org is a terrible mess. A proper solution would
        // be any of those:
        //
        // 1. Somehow undo the brain damage that the C standard is, and write a sane C standard
        // instead. The semantics of such "sane C" would include what '-fno-strict-aliasing' and
        // '-fwrapv' options to gcc do.
        //
        // 2. Force X upstream to provide headers with '__attribute__((__may_alias__))' on
        // appropriate structures/unions. Well, *at least* if gcc or clang is used - nobody
        // actually uses anything else.
        //

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
    if (dpy) {
        XCloseDisplay(dpy);
    }
    ls_strarr_destroy(groups);
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
