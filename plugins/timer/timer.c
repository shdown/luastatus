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
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "libls/alloc_utils.h"
#include "libls/cstring_utils.h"
#include "libls/evloop_utils.h"

typedef struct {
    double period;
    char *fifo;
    LSPushedTimeout pushed_tmo;
} Priv;

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->fifo);
    ls_pushed_timeout_destroy(&p->pushed_tmo);
    free(p);
}

static int init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .period = 1.0,
        .fifo = NULL,
    };
    ls_pushed_timeout_init(&p->pushed_tmo);

    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse period
    if (moon_visit_num(&mv, -1, "period", &p->period, true) < 0)
        goto mverror;
    if (p->period < 0) {
        LS_FATALF(pd, "period is invalid");
        goto error;
    }

    // Parse fifo
    if (moon_visit_str(&mv, -1, "fifo", &p->fifo, NULL, true) < 0)
        goto mverror;

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
    ls_pushed_timeout_push_luafunc(&p->pushed_tmo, L); // L: table func
    lua_setfield(L, -2, "push_period"); // L: table
}

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    int fifo_fd = -1;

    const char *what = "hello";

    while (1) {
        lua_State *L = funcs.call_begin(pd->userdata);
        lua_pushstring(L, what);
        funcs.call_end(pd->userdata);

        if (ls_fifo_open(&fifo_fd, p->fifo) < 0) {
            LS_WARNF(pd, "ls_fifo_open: %s: %s", p->fifo, LS_FIFO_STRERROR_ONSTACK(errno));
        }
        double tmo = ls_pushed_timeout_fetch(&p->pushed_tmo, p->period);
        int r = ls_fifo_wait(&fifo_fd, tmo);
        if (r < 0) {
            LS_FATALF(pd, "ls_fifo_wait: %s", ls_strerror_onstack(errno));
            goto error;
        } else if (r == 0) {
            what = "timeout";
        } else {
            what = "fifo";
        }
    }

error:
    close(fifo_fd);
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .run = run,
    .destroy = destroy,
};
