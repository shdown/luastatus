#include "include/plugin.h"
#include "include/plugin_logf_macros.h"
#include "include/plugin_utils.h"

#include <lua.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>

#include "libls/alloc_utils.h"
#include "libls/compdep.h"
#include "libls/strarr.h"

#include "rules_names.h"
#include "parse_groups.h"

typedef struct {
    char *dpyname;
    unsigned deviceid;
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
LuastatusPluginInitResult
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .dpyname = NULL,
        .deviceid = XkbUseCoreKbd,
    };

    PU_MAYBE_VISIT_STR("display", s,
        p->dpyname = ls_xstrdup(s);
    );

    PU_MAYBE_VISIT_NUM("device_id", n,
        if (n < 0) {
            LUASTATUS_FATALF(pd, "device_id < 0");
            goto error;
        }
        if (n > UINT_MAX) {
            LUASTATUS_FATALF(pd, "device_id > UINT_MAX");
            goto error;
        }
        p->deviceid = n;
    );

    return LUASTATUS_PLUGIN_INIT_RESULT_OK;

error:
    destroy(pd);
    return LUASTATUS_PLUGIN_INIT_RESULT_ERR;
}

Display *
open_dpy(LuastatusPluginData *pd, char *dpyname)
{
    XkbIgnoreExtension(False);
    int event_out;
    int error_out;
    int reason_out;
    int major_in_out = XkbMajorVersion;
    int minor_in_out = XkbMinorVersion;
    Display *dpy = XkbOpenDisplay(dpyname, &event_out, &error_out, &major_in_out, &minor_in_out,
                                  &reason_out);
    if (dpy && reason_out == XkbOD_Success) {
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
    LUASTATUS_FATALF(pd, "XkbOpenDisplay failed: %s", msg);
    return NULL;
}

bool
query_groups(Display *dpy, LSStringArray *groups)
{
    RulesNames rn;
    if (!rules_names_load(dpy, &rn)) {
        return false;
    }
    parse_groups(groups, rn.layout ? rn.layout : "");
    rules_names_destroy(&rn);
    return true;
}

static LuastatusPluginData *global_pd;
static jmp_buf global_jmpbuf;

// https://tronche.com/gui/x/xlib/event-handling/protocol-errors/XSetIOErrorHandler.html
//
// > Xlib calls the program's supplied error handler if any sort of system call
// > error occurs (for example, the connection to the server was lost). This is
// > assumed to be a fatal condition, and the called routine should not return.
// > If the I/O error handler does return, the client process exits.
int
x11_io_error_handler(LS_ATTR_UNUSED_ARG Display *dpy)
{
    LUASTATUS_FATALF(global_pd, "X11 I/O error occurred");
    longjmp(global_jmpbuf, 1);
}

// https://tronche.com/gui/x/xlib/event-handling/protocol-errors/XSetErrorHandler.html
//
// > Because this condition is not assumed to be fatal, it is acceptable for
// > your error handler to return; the returned value is ignored.
int
x11_error_handler(LS_ATTR_UNUSED_ARG Display *dpy, XErrorEvent *ev)
{
    LUASTATUS_ERRF(global_pd,
                   "X11 error: serial=%ld, error_code=%d, request_code=%d, minor_code=%d",
                   ev->serial, ev->error_code, ev->request_code, ev->minor_code);
    return 0;
}

static
void
run(
    LuastatusPluginData *pd,
    LuastatusPluginCallBegin call_begin,
    LuastatusPluginCallEnd call_end)
{
    Priv *p = pd->priv;
    LSStringArray groups = ls_strarr_new();
    Display *dpy = NULL;

    global_pd = pd;
    if (setjmp(global_jmpbuf) != 0) {
        // We have jumped here.
        // We don't bother to clean up because we could not call XCloseDisplay(dpy) anyway.
        // Since we have a taint and there can't be multiple xkb widgets, the amount of leaked
        // memory is constant.
        return;
    }
    XSetIOErrorHandler(x11_io_error_handler);
    XSetErrorHandler(x11_error_handler);

    if (!(dpy = open_dpy(pd, p->dpyname))) {
        goto error;
    }

    if (!query_groups(dpy, &groups)) {
        LUASTATUS_FATALF(pd, "query_groups failed");
        goto error;
    }
    while (1) {
        // query current state
        XkbStateRec state;
        if (XkbGetState(dpy, p->deviceid, &state) != Success) {
            LUASTATUS_FATALF(pd, "XkbGetState failed");
            goto error;
        }

        // check if group is valid and possibly requery
        int group = state.group;
        if (group < 0) {
            LUASTATUS_WARNF(pd, "group ID is negative (%d)", group);
        } else if ((size_t) group >= ls_strarr_size(groups)) {
            LUASTATUS_WARNF(pd, "group ID (%d) is too large, requerying", group);
            if (!query_groups(dpy, &groups)) {
                LUASTATUS_FATALF(pd, "query_groups failed");
                goto error;
            }
            if ((size_t) group >= ls_strarr_size(groups)) {
                LUASTATUS_WARNF(pd, "group ID is still too large");
            }
        }

        // make a call
        lua_State *L = call_begin(pd->userdata); // L: -
        lua_newtable(L); // L: table
        lua_pushinteger(L, group); // L: table n
        lua_setfield(L, -2, "id"); // L: table
        if (group >= 0 && (size_t) group < ls_strarr_size(groups)) {
            size_t nbuf;
            const char *buf = ls_strarr_at(groups, group, &nbuf);
            lua_pushlstring(L, buf, nbuf); // L: table group
            lua_setfield(L, -2, "name"); // L: table
        }
        call_end(pd->userdata);

        // wait for next event
        if (XkbSelectEventDetails(dpy, p->deviceid, XkbStateNotify, XkbAllStateComponentsMask,
                                  XkbGroupStateMask)
            == False)
        {
            LUASTATUS_FATALF(pd, "XkbSelectEventDetails failed");
            goto error;
        }
        XEvent event;
        // XXX should we block all signals here to ensure XNextEvent will not
        // fail with EINTR? Apparently not: XNextEvent is untimed, so there is
        // no sense for it to use a multiplexing interface.
        XNextEvent(dpy, &event);
    }

error:
    if (dpy) {
        XCloseDisplay(dpy);
    }
    ls_strarr_destroy(groups);
}

LuastatusPluginIface luastatus_plugin_iface = {
    .init = init,
    .run = run,
    .destroy = destroy,
    .taints = (const char *[]) {"libx11", NULL},
};
