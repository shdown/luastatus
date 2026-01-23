/*
 * Copyright (C) 2015-2026  luastatus developers
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
#include <lauxlib.h>
#include <stdlib.h>
#include <sys/statvfs.h>
#include <pthread.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "libls/ls_alloc_utils.h"
#include "libls/ls_strarr.h"
#include "libls/ls_fifo_device.h"
#include "libls/ls_tls_ebuf.h"
#include "libls/ls_time_utils.h"
#include "libls/ls_panic.h"
#include "libls/ls_assert.h"

#include "strlist.h"

enum { MAX_DYN_PATHS = 256 };

typedef struct {
    LS_StringArray paths;
    LS_StringArray globs;

    double period;
    char *fifo;

    bool dyn_paths_enabled;
    Strlist dyn_paths;
    pthread_mutex_t dyn_mtx;
} Priv;

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    ls_strarr_destroy(p->paths);
    ls_strarr_destroy(p->globs);
    free(p->fifo);

    if (p->dyn_paths_enabled) {
        strlist_destroy(p->dyn_paths);
        LS_PTH_CHECK(pthread_mutex_destroy(&p->dyn_mtx));
    }

    free(p);
}

static int parse_paths_elem(MoonVisit *mv, void *ud, int kpos, int vpos)
{
    mv->where = "'paths' element";
    (void) kpos;

    Priv *p = ud;

    if (moon_visit_checktype_at(mv, NULL, vpos, LUA_TSTRING) < 0)
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

    if (moon_visit_checktype_at(mv, NULL, vpos, LUA_TSTRING) < 0)
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
        .dyn_paths_enabled = false,
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
    if (!ls_double_is_good_time_delta(p->period)) {
        LS_FATALF(pd, "period is invalid");
        goto error;
    }

    // Parse fifo
    if (moon_visit_str(&mv, -1, "fifo", &p->fifo, NULL, true) < 0)
        goto mverror;

    // Parse enable_dyn_paths
    bool enable_dyn_paths = false;
    if (moon_visit_bool(&mv, -1, "enable_dyn_paths", &enable_dyn_paths, true) < 0) {
        goto mverror;
    }
    if (enable_dyn_paths) {
        p->dyn_paths_enabled = true;
        p->dyn_paths = strlist_new(MAX_DYN_PATHS);
        LS_PTH_CHECK(pthread_mutex_init(&p->dyn_mtx, NULL));
    }

    // Warn if both paths and globs are empty and /enable_dyn_paths/ is false
    if (!enable_dyn_paths && !ls_strarr_size(p->paths) && !ls_strarr_size(p->globs)) {
        LS_WARNF(pd, "both paths and globs are empty, and enable_dyn_paths is false");
    }

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
        LS_WARNF(pd, "statvfs: %s: %s", path, ls_tls_strerror(errno));
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

typedef struct {
    lua_State *L;
    LuastatusPluginData *pd;
    LuastatusPluginRunFuncs funcs;
} Call;

static inline Call start_call(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    lua_State *L = funcs.call_begin(pd->userdata);
    lua_newtable(L);

    return (Call) {
        .L = L,
        .pd = pd,
        .funcs = funcs,
    };
}

static inline void add_path_to_call(Call c, const char *path)
{
    LS_ASSERT(path != NULL);

    if (push_for(c.pd, c.L, path)) {
        lua_setfield(c.L, -2, path);
    }
}

static inline void end_call(Call c)
{
    c.funcs.call_end(c.pd->userdata);
}

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    LS_FifoDevice dev = ls_fifo_device_new();

    LS_TimeDelta TD = ls_double_to_TD_or_die(p->period);

    while (1) {
        // make a call
        Call call = start_call(pd, funcs);

        for (size_t i = 0; i < ls_strarr_size(p->paths); ++i) {
            add_path_to_call(call, ls_strarr_at(p->paths, i, NULL));
        }

        if (p->dyn_paths_enabled) {
            LS_PTH_CHECK(pthread_mutex_lock(&p->dyn_mtx));
            for (size_t i = 0; i < p->dyn_paths.size; ++i) {
                add_path_to_call(call, p->dyn_paths.data[i]);
            }
            LS_PTH_CHECK(pthread_mutex_unlock(&p->dyn_mtx));
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
                add_path_to_call(call, gbuf.gl_pathv[j]);
            }
            globfree(&gbuf);
        }

        end_call(call);

        // wait
        if (ls_fifo_device_open(&dev, p->fifo) < 0) {
            LS_WARNF(pd, "ls_fifo_device_open: %s: %s", p->fifo, ls_tls_strerror(errno));
        }
        if (ls_fifo_device_wait(&dev, TD) < 0) {
            LS_FATALF(pd, "ls_fifo_device_wait: %s: %s", p->fifo, ls_tls_strerror(errno));
            goto error;
        }
    }

error:
    ls_fifo_device_close(&dev);
}

static int lfunc_add_dyn_path(lua_State *L)
{
    Priv *p = lua_touserdata(L, lua_upvalueindex(1));
    const char *path = luaL_checkstring(L, 1);

    LS_ASSERT(p->dyn_paths_enabled);

    LS_PTH_CHECK(pthread_mutex_lock(&p->dyn_mtx));
    int rc = strlist_push(&p->dyn_paths, path);
    LS_PTH_CHECK(pthread_mutex_unlock(&p->dyn_mtx));

    if (rc < 0) {
        return luaL_error(L, "no place for new path");
    }
    lua_pushboolean(L, rc);
    return 1;
}

static int lfunc_remove_dyn_path(lua_State *L)
{
    Priv *p = lua_touserdata(L, lua_upvalueindex(1));
    const char *path = luaL_checkstring(L, 1);

    LS_ASSERT(p->dyn_paths_enabled);

    LS_PTH_CHECK(pthread_mutex_lock(&p->dyn_mtx));
    int rc = strlist_remove(&p->dyn_paths, path);
    LS_PTH_CHECK(pthread_mutex_unlock(&p->dyn_mtx));

    lua_pushboolean(L, rc);
    return 1;
}

static int lfunc_get_max_dyn_paths(lua_State *L)
{
    lua_pushinteger(L, MAX_DYN_PATHS);
    return 1;
}

static void register_funcs(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv;

    if (!p->dyn_paths_enabled) {
        return;
    }

    // L: ? table

    lua_pushlightuserdata(L, p); // L: ? table ud
    lua_pushcclosure(L, lfunc_add_dyn_path, 1); // L: ? table func
    lua_setfield(L, -2, "add_dyn_path"); // L: ? table

    lua_pushlightuserdata(L, p); // L: ? table ud
    lua_pushcclosure(L, lfunc_remove_dyn_path, 1); // L: ? table func
    lua_setfield(L, -2, "remove_dyn_path"); // L: ? table

    lua_pushcfunction(L, lfunc_get_max_dyn_paths); // L: ? table func
    lua_setfield(L, -2, "get_max_dyn_paths"); // L: ? table
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .run = run,
    .destroy = destroy,
};
