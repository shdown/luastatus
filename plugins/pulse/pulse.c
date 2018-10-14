#include <lua.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <pulse/pulseaudio.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"
#include "include/plugin_utils.h"

#include "libls/alloc_utils.h"
#include "libls/osdep.h"
#include "libls/io_utils.h"
#include "libls/errno_utils.h"

// Note: some parts of this file are stolen from i3status' src/pulse.c.
// This is fine since the BSD 3-Clause licence, under which it is licenced, is compatible with
// LGPL-3.0.

typedef struct {
    char *sink_name;
    int self_pipe[2];
} Priv;

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->sink_name);
    ls_close(p->self_pipe[0]);
    ls_close(p->self_pipe[1]);
    free(p);
}

static
int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .sink_name = NULL,
        .self_pipe = {-1, -1},
    };

    PU_MAYBE_VISIT_STR("sink", NULL, s,
        p->sink_name = ls_xstrdup(s);
    );
    if (!p->sink_name) {
        p->sink_name = ls_xstrdup("@DEFAULT_SINK@");
    }

    PU_MAYBE_VISIT_BOOL("make_self_pipe", NULL, b,
        if (b) {
            if (ls_cloexec_pipe(p->self_pipe) < 0) {
                LS_WITH_ERRSTR(s, errno,
                    LS_FATALF(pd, "ls_cloexec_pipe: %s", s);
                );
                p->self_pipe[0] = -1;
                p->self_pipe[1] = -1;
                goto error;
            }
            ls_make_nonblock(p->self_pipe[0]);
            ls_make_nonblock(p->self_pipe[1]);
        }
    );

    return LUASTATUS_OK;

error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static
int
l_wake_up(lua_State *L)
{
    LuastatusPluginData *pd = lua_touserdata(L, lua_upvalueindex(1));
    Priv *p = pd->priv;

    write(p->self_pipe[1], "", 1); // write '\0'

    return 0;
}

static
void
register_funcs(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv;
    if (p->self_pipe[0] >= 0) {
        // L: table
        lua_pushlightuserdata(L, pd); // L: table bd
        lua_pushcclosure(L, l_wake_up, 1); // L: table pd l_wake_up
        ls_lua_rawsetf(L, "wake_up"); // L: table
    }
}

typedef struct {
    LuastatusPluginData *pd;
    LuastatusPluginRunFuncs funcs;
    pa_mainloop *ml;
    uint32_t sink_idx;
} UserData;

static
void
self_pipe_cb(pa_mainloop_api *api, pa_io_event *e, int fd, pa_io_event_flags_t events, void *vud)
{
    (void) api;
    (void) e;
    (void) events;

    read(fd, (char[1]) {'\0'}, 1);

    UserData *ud = vud;
    lua_State *L = ud->funcs.call_begin(ud->pd->userdata);
    lua_pushnil(L);
    ud->funcs.call_end(ud->pd->userdata);
}

static
void
store_volume_from_sink_cb(pa_context *c, const pa_sink_info *info, int eol, void *vud)
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
            lua_newtable(L); // L: ? table
            lua_pushinteger(L, pa_cvolume_avg(&info->volume)); // L: ? table integer
            lua_setfield(L, -2, "cur"); // L: ? table
            lua_pushinteger(L, PA_VOLUME_NORM); // L: ? table integer
            lua_setfield(L, -2, "norm"); // L: ? table
            lua_pushboolean(L, info->mute); // L: ? table boolean
            lua_setfield(L, -2, "mute"); // L: ? table
            ud->funcs.call_end(ud->pd->userdata);
        }
    }
}

static
void
store_sink_cb(pa_context *c, const pa_sink_info *info, int eol, void *vud)
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

static
void
update_sink(pa_context *c, void *vud)
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

static
void
subscribe_cb(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *vud)
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

static
void
context_state_cb(pa_context *c, void *vud)
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

static
bool
iteration(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
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

    if (p->self_pipe[0] >= 0) {
        pipe_ev = api->io_new(api, p->self_pipe[0], PA_IO_EVENT_INPUT, self_pipe_cb, &ud);
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

static
void
run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    while (1) {
        if (!iteration(pd, funcs)) {
            nanosleep((struct timespec[1]) {{.tv_sec = 5}}, NULL);
        }
    }
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .run = run,
    .destroy = destroy,
};
