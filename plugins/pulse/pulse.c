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
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <pulse/pulseaudio.h>
#include <pulse/version.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "libls/alloc_utils.h"
#include "libls/tls_ebuf.h"
#include "libls/evloop_lfuncs.h"
#include "libls/time_utils.h"

#ifdef PA_CHECK_VERSION
# define MY_CHECK_VERSION(A_, B_, C_) PA_CHECK_VERSION(A_, B_, C_)
#else
# define MY_CHECK_VERSION(A_, B_, C_) 0
#endif

// Note: some parts of this file are stolen from i3status' src/pulse.c.
// This is fine since the BSD 3-Clause licence, under which it is licenced, is compatible with
// LGPL-3.0.

typedef struct {
    char *sink_name;
    int pipefds[2];
} Priv;

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->sink_name);
    close(p->pipefds[0]);
    close(p->pipefds[1]);
    free(p);
}

static int init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .sink_name = NULL,
        .pipefds = {-1, -1},
    };
    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse sink
    if (moon_visit_str(&mv, -1, "sink", &p->sink_name, NULL, true) < 0)
        goto mverror;
    if (!p->sink_name)
        p->sink_name = ls_xstrdup("@DEFAULT_SINK@");

    // Parse make_self_pipe
    bool mkpipe = false;
    if (moon_visit_bool(&mv, -1, "make_self_pipe", &mkpipe, true) < 0)
        goto mverror;
    if (mkpipe) {
        if (ls_self_pipe_open(p->pipefds) < 0) {
            LS_FATALF(pd, "ls_self_pipe_open: %s", ls_tls_strerror(errno));
            goto error;
        }
    }

    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static void register_funcs(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv;
    // L: table
    ls_self_pipe_push_luafunc(p->pipefds, L); // L: table func
    lua_setfield(L, -2, "wake_up"); // L: table
}

typedef struct {
    LuastatusPluginData *pd;
    LuastatusPluginRunFuncs funcs;
    pa_mainloop *ml;
    uint32_t sink_idx;
} UserData;

static void self_pipe_cb(
        pa_mainloop_api *api,
        pa_io_event *e,
        int fd,
        pa_io_event_flags_t events,
        void *vud)
{
    (void) api;
    (void) e;
    (void) events;

    char c;
    ssize_t unused = read(fd, &c, 1);
    (void) unused;

    UserData *ud = vud;
    lua_State *L = ud->funcs.call_begin(ud->pd->userdata);
    lua_pushnil(L);
    ud->funcs.call_end(ud->pd->userdata);
}

#if MY_CHECK_VERSION(14, 0, 0)
static void push_port_type(lua_State *L, int type)
{
    switch (type) {
    default:                              lua_pushstring(L, "unknown"); break;
    case PA_DEVICE_PORT_TYPE_UNKNOWN:     lua_pushstring(L, "unknown"); break;
    case PA_DEVICE_PORT_TYPE_AUX:         lua_pushstring(L, "aux"); break;
    case PA_DEVICE_PORT_TYPE_SPEAKER:     lua_pushstring(L, "speaker"); break;
    case PA_DEVICE_PORT_TYPE_HEADPHONES:  lua_pushstring(L, "headphones"); break;
    case PA_DEVICE_PORT_TYPE_LINE:        lua_pushstring(L, "line"); break;
    case PA_DEVICE_PORT_TYPE_MIC:         lua_pushstring(L, "mic"); break;
    case PA_DEVICE_PORT_TYPE_HEADSET:     lua_pushstring(L, "headset"); break;
    case PA_DEVICE_PORT_TYPE_HANDSET:     lua_pushstring(L, "handset"); break;
    case PA_DEVICE_PORT_TYPE_EARPIECE:    lua_pushstring(L, "earpiece"); break;
    case PA_DEVICE_PORT_TYPE_SPDIF:       lua_pushstring(L, "spdif"); break;
    case PA_DEVICE_PORT_TYPE_HDMI:        lua_pushstring(L, "hdmi"); break;
    case PA_DEVICE_PORT_TYPE_TV:          lua_pushstring(L, "tv"); break;
    case PA_DEVICE_PORT_TYPE_RADIO:       lua_pushstring(L, "radio"); break;
    case PA_DEVICE_PORT_TYPE_VIDEO:       lua_pushstring(L, "video"); break;
    case PA_DEVICE_PORT_TYPE_USB:         lua_pushstring(L, "usb"); break;
    case PA_DEVICE_PORT_TYPE_BLUETOOTH:   lua_pushstring(L, "bluetooth"); break;
    case PA_DEVICE_PORT_TYPE_PORTABLE:    lua_pushstring(L, "portable"); break;
    case PA_DEVICE_PORT_TYPE_HANDSFREE:   lua_pushstring(L, "handsfree"); break;
    case PA_DEVICE_PORT_TYPE_CAR:         lua_pushstring(L, "car"); break;
    case PA_DEVICE_PORT_TYPE_HIFI:        lua_pushstring(L, "hifi"); break;
    case PA_DEVICE_PORT_TYPE_PHONE:       lua_pushstring(L, "phone"); break;
    case PA_DEVICE_PORT_TYPE_NETWORK:     lua_pushstring(L, "network"); break;
    case PA_DEVICE_PORT_TYPE_ANALOG:      lua_pushstring(L, "analog"); break;
    }
}
#endif

static void store_volume_from_sink_cb(
        pa_context *c,
        const pa_sink_info *info,
        int eol,
        void *vud)
{
    UserData *ud = vud;
    if (eol < 0) {
        if (pa_context_errno(c) == PA_ERR_NOENTITY) {
            return;
        }
        LS_ERRF(ud->pd, "PulseAudio error: %s", pa_strerror(pa_context_errno(c)));
    } else if (eol == 0) {
        if (info->index == ud->sink_idx) {
            lua_State *L = ud->funcs.call_begin(ud->pd->userdata);
            // L: ?
            lua_createtable(L, 0, 7); // L: ? table
            lua_pushinteger(L, pa_cvolume_avg(&info->volume)); // L: ? table integer
            lua_setfield(L, -2, "cur"); // L: ? table
            lua_pushinteger(L, PA_VOLUME_NORM); // L: ? table integer
            lua_setfield(L, -2, "norm"); // L: ? table
            lua_pushboolean(L, info->mute); // L: ? table boolean
            lua_setfield(L, -2, "mute"); // L: ? table
            lua_pushinteger(L, info->index); // L: ? table integer
            lua_setfield(L, -2, "index"); // L: ? table
            lua_pushstring(L, info->name); // L: ? table string
            lua_setfield(L, -2, "name"); // L: ? table
            lua_pushstring(L, info->description); // L: ? table string
            lua_setfield(L, -2, "desc"); // L: ? table

#if MY_CHECK_VERSION(0, 9, 16)
            if (!info->active_port) {
                lua_pushnil(L); // L: ? table nil
            } else {
                lua_createtable(L, 0, 3); // L: ? table table
                lua_pushstring(L, info->active_port->name); // L: ? table table string
                lua_setfield(L, -2, "name"); // L: ? table table
                lua_pushstring(L, info->active_port->description); // L: ? table table string
                lua_setfield(L, -2, "desc"); // L: ? table table
# if MY_CHECK_VERSION(14, 0, 0)
                push_port_type(L, info->active_port->type); // L: ? table table string
                lua_setfield(L, -2, "type"); // L: ? table table
# endif
            }
            lua_setfield(L, -2, "port"); // L: ? table
#endif

            ud->funcs.call_end(ud->pd->userdata);
        }
    }
}

static void store_sink_cb(
        pa_context *c,
        const pa_sink_info *info,
        int eol,
        void *vud)
{
    UserData *ud = vud;
    if (info) {
        if (ud->sink_idx != info->index) {
            // sink has changed?
            ud->sink_idx = info->index;
            store_volume_from_sink_cb(c, info, eol, vud);
        }
    }
}

static void update_sink(pa_context *c, void *vud)
{
    UserData *ud = vud;
    Priv *p = ud->pd->priv;

    pa_operation *o = pa_context_get_sink_info_by_name(c, p->sink_name, store_sink_cb, vud);
    if (o) {
        pa_operation_unref(o);
    } else {
        LS_ERRF(ud->pd, "pa_context_get_sink_info_by_name: %s", pa_strerror(pa_context_errno(c)));
    }
}

static void subscribe_cb(
        pa_context *c,
        pa_subscription_event_type_t t,
        uint32_t idx,
        void *vud)
{
    UserData *ud = vud;

    if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) != PA_SUBSCRIPTION_EVENT_CHANGE) {
        return;
    }
    pa_subscription_event_type_t facility = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
    switch (facility) {
    case PA_SUBSCRIPTION_EVENT_SERVER:
        // server change event, see if the sink has changed
        update_sink(c, vud);
        break;
    case PA_SUBSCRIPTION_EVENT_SINK:
        {
            pa_operation *o = pa_context_get_sink_info_by_index(
                c, idx, store_volume_from_sink_cb, vud);

            if (o) {
                pa_operation_unref(o);
            } else {
                LS_ERRF(ud->pd, "pa_context_get_sink_info_by_index: %s",
                        pa_strerror(pa_context_errno(c)));
            }
        }
        break;
    default:
        break;
    }
}

static void context_state_cb(pa_context *c, void *vud)
{
    UserData *ud = vud;
    switch (pa_context_get_state(c)) {
    case PA_CONTEXT_UNCONNECTED:
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
    case PA_CONTEXT_TERMINATED:
    default:
        break;

    case PA_CONTEXT_READY:
        {
            pa_context_set_subscribe_callback(c, subscribe_cb, vud);
            update_sink(c, vud);
            pa_operation *o = pa_context_subscribe(
                c, PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SERVER, NULL, NULL);

            if (o) {
                pa_operation_unref(o);
            } else {
                LS_ERRF(ud->pd, "pa_context_subscribe: %s", pa_strerror(pa_context_errno(c)));
            }
        }
        break;

    case PA_CONTEXT_FAILED:
        // server disconnected us
        pa_mainloop_quit(ud->ml, 1);
        break;
    }
}

static bool iteration(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    bool ret = false;
    Priv *p = pd->priv;
    UserData ud = {.pd = pd, .funcs = funcs, .sink_idx = UINT32_MAX};
    pa_mainloop_api *api = NULL;
    pa_context *ctx = NULL;
    pa_io_event *pipe_ev = NULL;

    if (!(ud.ml = pa_mainloop_new())) {
        LS_FATALF(pd, "pa_mainloop_new() failed");
        goto error;
    }
    if (!(api = pa_mainloop_get_api(ud.ml))) {
        LS_FATALF(pd, "pa_mainloop_get_api() failed");
        goto error;
    }
    pa_proplist *proplist = pa_proplist_new();
    pa_proplist_sets(proplist, PA_PROP_APPLICATION_NAME, "luastatus-plugin-pulse");
    pa_proplist_sets(proplist, PA_PROP_APPLICATION_ID, "io.github.shdown.luastatus");
    pa_proplist_sets(proplist, PA_PROP_APPLICATION_VERSION, "0.0.1");
    ctx = pa_context_new_with_proplist(api, "luastatus-plugin-pulse", proplist);
    pa_proplist_free(proplist);
    if (!ctx) {
        LS_FATALF(pd, "pa_context_new_with_proplist() failed");
        goto error;
    }

    pa_context_set_state_callback(ctx, context_state_cb, &ud);
    if (pa_context_connect(ctx, NULL, PA_CONTEXT_NOFAIL | PA_CONTEXT_NOAUTOSPAWN, NULL) < 0) {
        LS_FATALF(pd, "pa_context_connect: %s", pa_strerror(pa_context_errno(ctx)));
        goto error;
    }

    if (p->pipefds[0] >= 0) {
        pipe_ev = api->io_new(api, p->pipefds[0], PA_IO_EVENT_INPUT, self_pipe_cb, &ud);
        if (!pipe_ev) {
            LS_FATALF(pd, "io_new() failed");
            goto error;
        }
    }

    ret = true;

    int ignored;
    if (pa_mainloop_run(ud.ml, &ignored) < 0) {
        LS_FATALF(pd, "pa_mainloop_run: %s", pa_strerror(pa_context_errno(ctx)));
        goto error;
    }

error:
    if (pipe_ev) {
        assert(api);
        api->io_free(pipe_ev);
    }
    if (ctx) {
        pa_context_unref(ctx);
    }
    if (ud.ml) {
        pa_mainloop_free(ud.ml);
    }
    return ret;
}

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    for (;;) {
        if (!iteration(pd, funcs)) {
            ls_sleep(5.0);
        }
    }
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .run = run,
    .destroy = destroy,
};
