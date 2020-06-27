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

#include <sys/select.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <lua.h>
#include <stdlib.h>
#include <libudev.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "libls/alloc_utils.h"
#include "libls/time_utils.h"
#include "libls/cstring_utils.h"
#include "libls/evloop_utils.h"

typedef struct {
    char *subsystem;
    char *devtype;
    char *tag;
    bool kernel_ev;
    bool greet;
    double tmo;
    LSPushedTimeout pushed_tmo;
} Priv;

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->subsystem);
    free(p->devtype);
    free(p->tag);
    ls_pushed_timeout_destroy(&p->pushed_tmo);
    free(p);
}

static
int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .subsystem = NULL,
        .devtype = NULL,
        .tag = NULL,
        .kernel_ev = false,
        .greet = false,
        .tmo = -1,
    };
    ls_pushed_timeout_init(&p->pushed_tmo);

    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse subsystem
    if (moon_visit_str(&mv, -1, "subsystem", &p->subsystem, NULL, true) < 0)
        goto mverror;

    // Parse devtype
    if (moon_visit_str(&mv, -1, "devtype", &p->devtype, NULL, true) < 0)
        goto mverror;

    // Parse tag
    if (moon_visit_str(&mv, -1, "tag", &p->tag, NULL, true) < 0)
        goto mverror;

    // Parse kernel_events
    if (moon_visit_bool(&mv, -1, "kernel_events", &p->kernel_ev, true) < 0)
        goto mverror;

    // Parse timeout
    if (moon_visit_num(&mv, -1, "timeout", &p->tmo, true) < 0)
        goto mverror;

    // Parse greet
    if (moon_visit_bool(&mv, -1, "greet", &p->greet, true) < 0)
        goto mverror;

    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
//error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static
void
register_funcs(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv;
    // L: table
    ls_pushed_timeout_push_luafunc(&p->pushed_tmo, L); // L: table func
    lua_setfield(L, -2, "push_timeout"); // L: table
}

static inline
void
report_status(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs, const char *status)
{
    lua_State *L = funcs.call_begin(pd->userdata);
    lua_createtable(L, 0, 1); // L: table
    lua_pushstring(L, status); // L: table string
    lua_setfield(L, -2, "what"); // L: table
    funcs.call_end(pd->userdata);
}

static
void
report_event(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs, struct udev_device *dev)
{
    lua_State *L = funcs.call_begin(pd->userdata);
    lua_createtable(L, 0, 4); // L: table

    lua_pushstring(L, "event"); // L: table string
    lua_setfield(L, -2, "what"); // L: table

#define PROP(Key_, Func_) \
    do { \
        /* L: table */ \
        const char *r_ = Func_(dev); \
        if (r_) { \
            lua_pushstring(L, r_);     /* L: table string */ \
            lua_setfield(L, -2, Key_); /* L: table */ \
        } \
    } while (0)

    PROP("syspath", udev_device_get_syspath);
    PROP("sysname", udev_device_get_sysname);
    PROP("sysnum", udev_device_get_sysnum);
    PROP("devpath", udev_device_get_devpath);
    PROP("devnode", udev_device_get_devnode);
    PROP("devtype", udev_device_get_devtype);
    PROP("subsystem", udev_device_get_subsystem);
    PROP("driver", udev_device_get_driver);
    PROP("action", udev_device_get_action);

#undef PROP

    funcs.call_end(pd->userdata);
}

static
void
run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    struct udev *udev = udev_new();
    if (!udev) {
        LS_FATALF(pd, "udev_new() failed");
        return;
    }

    struct udev_monitor *mon = udev_monitor_new_from_netlink(
        udev, p->kernel_ev ? "kernel" : "udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon, p->subsystem, p->devtype);
    udev_monitor_filter_add_match_tag(mon, p->tag);
    udev_monitor_enable_receiving(mon);
    const int fd = udev_monitor_get_fd(mon);

    if (p->greet) {
        report_status(pd, funcs, "hello");
    }

    while (1) {
        double tmo = ls_pushed_timeout_fetch(&p->pushed_tmo, p->tmo);
        int r = ls_wait_input_on_fd(fd, tmo);
        if (r < 0) {
            LS_FATALF(pd, "ls_wait_input_on_fd: %s", ls_strerror_onstack(errno));
            goto error;
        } else if (r == 0) {
            report_status(pd, funcs, "timeout");
        } else {
            struct udev_device *dev = udev_monitor_receive_device(mon);
            if (!dev) {
                // what the...?
                continue;
            }
            report_event(pd, funcs, dev);
            udev_device_unref(dev);
        }
    }
error:
    udev_unref(udev);
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .run = run,
    .destroy = destroy,
};
