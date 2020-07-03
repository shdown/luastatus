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

#include <errno.h>
#include <glob.h>
#include <stdbool.h>
#include <lua.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/statvfs.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "libls/alloc_utils.h"
#include "libls/strarr.h"
#include "libls/time_utils.h"
#include "libls/cstring_utils.h"
#include "libls/poll_utils.h"

typedef struct {
    LSStringArray paths;
    LSStringArray globs;
    double period;
    char *fifo;
} Priv;

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    ls_strarr_destroy(p->paths);
    ls_strarr_destroy(p->globs);
    free(p->fifo);
    free(p);
}

static int parse_paths_elem(MoonVisit *mv, void *ud, int kpos, int vpos)
{
    mv->where = "'paths' element";
    (void) kpos;

    Priv *p = ud;

    if (moon_visit_checktype_at(mv, "", vpos, LUA_TSTRING) < 0)
        return -1;

    const char *s = lua_tostring(mv->L, vpos);
    ls_strarr_append_s(&p->paths, s);
    return 1;
}

static int parse_globs_elem(MoonVisit *mv, void *ud, int kpos, int vpos)
{
    mv->where = "'globs' element";
    (void) kpos;

    Priv *p = ud;

    if (moon_visit_checktype_at(mv, "", vpos, LUA_TSTRING) < 0)
        return -1;

    const char *s = lua_tostring(mv->L, vpos);
    ls_strarr_append_s(&p->globs, s);
    return 1;
}

static int init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .paths = ls_strarr_new(),
        .globs = ls_strarr_new(),
        .period = 10.0,
        .fifo = NULL,
    };
    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse paths
    if (moon_visit_table_f(&mv, -1, "paths", parse_paths_elem, p, true) < 0)
        goto mverror;

    // Parse globs
    if (moon_visit_table_f(&mv, -1, "globs", parse_globs_elem, p, true) < 0)
        goto mverror;

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

    // Warn if both paths and globs are empty
    if (!ls_strarr_size(p->paths) && !ls_strarr_size(p->globs))
        LS_WARNF(pd, "both paths and globs are empty");

    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static bool push_for(LuastatusPluginData *pd, lua_State *L, const char *path)
{
    struct statvfs st;
    if (statvfs(path, &st) < 0) {
        LS_WARNF(pd, "statvfs: %s: %s", path, ls_strerror_onstack(errno));
        return false;
    }
    lua_createtable(L, 0, 3); // L: table
    lua_pushnumber(L, ((double) st.f_frsize) * st.f_blocks); // L: table n
    lua_setfield(L, -2, "total"); // L: table
    lua_pushnumber(L, ((double) st.f_frsize) * st.f_bfree); // L: table n
    lua_setfield(L, -2, "free"); // L: table
    lua_pushnumber(L, ((double) st.f_frsize) * st.f_bavail); // L: table n
    lua_setfield(L, -2, "avail"); // L: table
    return true;
}

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    int fifo_fd = -1;

    while (1) {
        // make a call
        lua_State *L = funcs.call_begin(pd->userdata);
        lua_newtable(L);
        for (size_t i = 0; i < ls_strarr_size(p->paths); ++i) {
            const char *path = ls_strarr_at(p->paths, i, NULL);
            if (push_for(pd, L, path)) {
                lua_setfield(L, -2, path);
            }
        }
        for (size_t i = 0; i < ls_strarr_size(p->globs); ++i) {
            const char *pattern = ls_strarr_at(p->globs, i, NULL);
            glob_t gbuf;
            switch (glob(pattern, GLOB_NOSORT, NULL, &gbuf)) {
            case 0:
            case GLOB_NOMATCH:
                break;
            default:
                LS_WARNF(pd, "glob() failed (out of memory?)");
            }
            for (size_t j = 0; j < gbuf.gl_pathc; ++j) {
                if (push_for(pd, L, gbuf.gl_pathv[j])) {
                    lua_setfield(L, -2, gbuf.gl_pathv[j]);
                }
            }
            globfree(&gbuf);
        }
        funcs.call_end(pd->userdata);
        // wait
        if (ls_fifo_open(&fifo_fd, p->fifo) < 0) {
            LS_WARNF(pd, "ls_fifo_open: %s: %s", p->fifo, LS_FIFO_STRERROR_ONSTACK(errno));
        }
        if (ls_fifo_wait(&fifo_fd, p->period) < 0) {
            LS_FATALF(pd, "ls_fifo_wait: %s: %s", p->fifo, ls_strerror_onstack(errno));
            goto error;
        }
    }

error:
    close(fifo_fd);
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
