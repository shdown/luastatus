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

#include <lua.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <poll.h>
#include <sys/types.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "libls/ls_alloc_utils.h"
#include "libls/ls_tls_ebuf.h"
#include "libls/ls_fifo_device.h"
#include "libls/ls_evloop_lfuncs.h"
#include "libls/ls_io_utils.h"
#include "libls/ls_time_utils.h"
#include "libprocalive/procalive_lfuncs.h"

typedef struct {
    double period;
    char *fifo;
    LS_PushedTimeout pushed_tmo;
    int self_pipe[2];
} Priv;

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;

    free(p->fifo);

    ls_pushed_timeout_destroy(&p->pushed_tmo);

    ls_close(p->self_pipe[0]);
    ls_close(p->self_pipe[1]);

    free(p);
}

static int init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .period = 1.0,
        .fifo = NULL,
        .self_pipe = {-1, -1},
    };
    ls_pushed_timeout_init(&p->pushed_tmo);

    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse period
    if (moon_visit_num(&mv, -1, "period", &p->period, true) < 0) {
        goto mverror;
    }
    if (!ls_double_is_good_time_delta(p->period)) {
        LS_FATALF(pd, "period is invalid");
        goto error;
    }

    // Parse fifo
    if (moon_visit_str(&mv, -1, "fifo", &p->fifo, NULL, true) < 0) {
        goto mverror;
    }

    // Parse make_self_pipe
    bool mkpipe = false;
    if (moon_visit_bool(&mv, -1, "make_self_pipe", &mkpipe, true) < 0) {
        goto mverror;
    }
    if (mkpipe) {
        if (ls_self_pipe_open(p->self_pipe) < 0) {
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
    // L: table
    procalive_lfuncs_register_all(L); // L: table

    Priv *p = pd->priv;
    ls_pushed_timeout_push_luafunc(&p->pushed_tmo, L); // L: table func
    lua_setfield(L, -2, "push_period"); // L: table

    ls_self_pipe_push_luafunc(p->self_pipe, L); // L: table func
    lua_setfield(L, -2, "wake_up"); // L: table
}

static inline void make_call(
    LuastatusPluginData *pd,
    LuastatusPluginRunFuncs funcs,
    const char *what)
{
    lua_State *L = funcs.call_begin(pd->userdata);
    lua_pushstring(L, what);
    funcs.call_end(pd->userdata);
}

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    LS_FifoDevice dev = ls_fifo_device_new();

    LS_TimeDelta default_tmo = ls_double_to_TD_or_die(p->period);

    make_call(pd, funcs, "hello");

    for (;;) {
        if (ls_fifo_device_open(&dev, p->fifo) < 0) {
            LS_WARNF(pd, "ls_fifo_device_open: %s: %s", p->fifo, ls_tls_strerror(errno));
        }
        LS_TimeDelta TD = ls_pushed_timeout_fetch(&p->pushed_tmo, default_tmo);

        struct pollfd pfds[2] = {
            {.fd = dev.fd,          .events = POLLIN},
            {.fd = p->self_pipe[0], .events = POLLIN},
        };
        int r = ls_poll(pfds, 2, TD);
        if (r < 0) {
            LS_FATALF(pd, "ls_poll: %s", ls_tls_strerror(errno));
            goto error;
        } else if (r == 0) {
            make_call(pd, funcs, "timeout");
        } else {
            if (pfds[0].revents) {
                make_call(pd, funcs, "fifo");
                ls_fifo_device_reset(&dev);

            } else if (pfds[1].revents) {
                char dummy;
                ssize_t nread = read(p->self_pipe[0], &dummy, 1);
                (void) nread;

                make_call(pd, funcs, "self_pipe");
            }
        }
    }

error:
    ls_fifo_device_close(&dev);
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .run = run,
    .destroy = destroy,
};
