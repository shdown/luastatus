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
#include <lauxlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
#include <sys/inotify.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "libls/alloc_utils.h"
#include "libls/cstring_utils.h"
#include "libls/vector.h"
#include "libls/evloop_utils.h"

#include "inotify_compat.h"

typedef struct {
    char *path;
    int wd;
} Watch;

typedef struct {
    int fd;
    LS_VECTOR_OF(Watch) init_watch;
    bool greet;
    double tmo;
    LSPushedTimeout pushed_tmo;
} Priv;

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    close(p->fd);
    for (size_t i = 0; i < p->init_watch.size; ++i) {
        free(p->init_watch.data[i].path);
    }
    LS_VECTOR_FREE(p->init_watch);
    ls_pushed_timeout_destroy(&p->pushed_tmo);
    free(p);
}

typedef struct {
    uint32_t mask;
    bool in, out;
    const char *name;
} EventType;

static const EventType EVENT_TYPES[] = {
    {IN_ACCESS,        true,  true,  "access"},
    {IN_ATTRIB,        true,  true,  "attrib"},
    {IN_CLOSE_WRITE,   true,  true,  "close_write"},
    {IN_CLOSE_NOWRITE, true,  true,  "close_nowrite"},
    {IN_CREATE,        true,  true,  "create"},
    {IN_DELETE,        true,  true,  "delete"},
    {IN_DELETE_SELF,   true,  true,  "delete_self"},
    {IN_MODIFY,        true,  true,  "modify"},
    {IN_MOVE_SELF,     true,  true,  "move_self"},
    {IN_MOVED_FROM,    true,  true,  "moved_from"},
    {IN_MOVED_TO,      true,  true,  "moved_to"},
    {IN_OPEN,          true,  true,  "open"},

    {IN_ALL_EVENTS,    true,  false, "all_events"},
    {IN_MOVE,          true,  false, "move"},
    {IN_CLOSE,         true,  false, "close"},
#ifdef IN_DONT_FOLLOW
    {IN_DONT_FOLLOW,   true,  false, "dont_follow"},
#endif
#ifdef IN_EXCL_UNLINK
    {IN_EXCL_UNLINK,   true,  false, "excl_unlink"},
#endif
    {IN_MASK_ADD,      true,  false, "mask_add"},
    {IN_ONESHOT,       true,  false, "oneshot"},
    {IN_ONLYDIR,       true,  false, "onlydir"},

    {IN_IGNORED,       false, true,  "ignored"},
    {IN_ISDIR,         false, true,  "isdir"},
    {IN_Q_OVERFLOW,    false, true,  "q_overflow"},
    {IN_UNMOUNT,       false, true,  "unmount"},

    {.name = NULL},
};

static int parse_evlist_elem(MoonVisit *mv, void *ud, int kpos, int vpos)
{
    mv->where = "element of event names list";
    (void) kpos;

    uint32_t *mask = ud;

    if (moon_visit_checktype_at(mv, "", vpos, LUA_TSTRING) < 0)
        goto error;

    const char *s = lua_tostring(mv->L, vpos);

    for (const EventType *et = EVENT_TYPES; et->name; ++et) {
        if (et->in && strcmp(et->name, s) == 0) {
            *mask |= et->mask;
            return 1;
        }
    }
    moon_visit_err(mv, "unknown input event '%s'", s);
error:
    return -1;
}

static int parse_watch_entry(MoonVisit *mv, void *ud, int kpos, int vpos)
{
    mv->where = "'watch' entry";

    LuastatusPluginData *pd = ud;
    Priv *p = pd->priv;

    // Parse key
    if (moon_visit_checktype_at(mv, "key", kpos, LUA_TSTRING) < 0)
        goto error;
    const char *path = lua_tostring(mv->L, kpos);

    // Parse value
    uint32_t mask = 0;
    if (moon_visit_table_f_at(mv, "value", vpos, parse_evlist_elem, &mask) < 0)
        goto error;

    // Add watch
    int wd = inotify_add_watch(p->fd, path, mask);
    if (wd < 0) {
        LS_ERRF(pd, "inotify_add_watch: %s: %s", path, ls_strerror_onstack(errno));
    } else {
        Watch w = {.path = ls_xstrdup(path), .wd = wd};
        LS_VECTOR_PUSH(p->init_watch, w);
    }
    return 1;
error:
    return -1;
}

static int init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .fd = -1,
        .init_watch = LS_VECTOR_NEW(),
        .greet = false,
        .tmo = -1,
    };
    ls_pushed_timeout_init(&p->pushed_tmo);

    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    if ((p->fd = compat_inotify_init(false, true)) < 0) {
        LS_FATALF(pd, "inotify_init: %s", ls_strerror_onstack(errno));
        goto error;
    }

    // Parse greet
    if (moon_visit_bool(&mv, -1, "greet", &p->greet, true) < 0)
        goto mverror;

    // Parse timeout
    if (moon_visit_num(&mv, -1, "timeout", &p->tmo, true) < 0)
        goto mverror;

    // Parse watch
    if (moon_visit_table_f(&mv, -1, "watch", parse_watch_entry, pd, false) < 0)
        goto mverror;

    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static int l_add_watch(lua_State *L)
{
    if (lua_gettop(L) != 2)
        return luaL_error(L, "expected exactly 2 arguments");

    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse first arg
    if (moon_visit_checktype_at(&mv, "first arg", 1, LUA_TSTRING) < 0)
        goto mverror;
    const char *path = lua_tostring(L, 1);

    // Parse second arg
    uint32_t mask = 0;
    if (moon_visit_table_f_at(&mv, "second arg", 2, parse_evlist_elem, &mask) < 0)
        goto mverror;

    // Add watch
    LuastatusPluginData *pd = lua_touserdata(L, lua_upvalueindex(1));
    Priv *p = pd->priv;

    int wd = inotify_add_watch(p->fd, path, mask);
    if (wd < 0) {
        LS_ERRF(pd, "inotify_add_watch: %s: %s", path, ls_strerror_onstack(errno));
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, wd);
    }
    return 1;

mverror:
    return luaL_error(L, "%s", errbuf);
}

static int l_remove_watch(lua_State *L)
{
    int wd = luaL_checkinteger(L, 1);

    LuastatusPluginData *pd = lua_touserdata(L, lua_upvalueindex(1));
    Priv *p = pd->priv;

    if (inotify_rm_watch(p->fd, wd) < 0) {
        LS_ERRF(pd, "inotify_rm_watch: %d: %s", wd, ls_strerror_onstack(errno));
        lua_pushboolean(L, false);
    } else {
        lua_pushboolean(L, true);
    }
    return 1;
}

static int l_get_initial_wds(lua_State *L)
{
    LuastatusPluginData *pd = lua_touserdata(L, lua_upvalueindex(1));
    Priv *p = pd->priv;

    lua_newtable(L); // L: table
    for (size_t i = 0; i < p->init_watch.size; ++i) {
        lua_pushinteger(L, p->init_watch.data[i].wd); // L: table wd
        lua_setfield(L, -2, p->init_watch.data[i].path); // L: table
    }
    return 1;
}

static void register_funcs(LuastatusPluginData *pd, lua_State *L)
{
    // L: table
    lua_pushlightuserdata(L, pd); // L: table pd
    lua_pushcclosure(L, l_add_watch, 1); // L: table closure
    lua_setfield(L, -2, "add_watch"); // L: table

    // L: table
    lua_pushlightuserdata(L, pd); // L: table pd
    lua_pushcclosure(L, l_remove_watch, 1); // L: table closure
    lua_setfield(L, -2, "remove_watch"); // L: table

    // L: table
    lua_pushlightuserdata(L, pd); // L: table pd
    lua_pushcclosure(L, l_get_initial_wds, 1); // L: table closure
    lua_setfield(L, -2, "get_initial_wds"); // L: table

    Priv *p = pd->priv;
    // L: table
    ls_pushed_timeout_push_luafunc(&p->pushed_tmo, L); // L: table func
    lua_setfield(L, -2, "push_timeout"); // L: table
}

static void push_event(lua_State *L, const struct inotify_event *event)
{
    // L: -
    lua_createtable(L, 0, 4); // L: table

    lua_pushstring(L, "event"); // L: table string
    lua_setfield(L, -2, "what"); // L: table

    lua_pushinteger(L, event->wd); // L: table wd
    lua_setfield(L, -2, "wd"); // L: table

    lua_newtable(L); // L: table table
    for (const EventType *et = EVENT_TYPES; et->name; ++et) {
        if (et->out && (event->mask & et->mask)) {
            lua_pushboolean(L, 1); // L: table table true
            lua_setfield(L, -2, et->name); // L: table table
        }
    }
    lua_setfield(L, -2, "mask"); // L: table

    lua_pushnumber(L, event->cookie); // L: table cookie
    lua_setfield(L, -2, "cookie"); // L: table

    if (event->len) {
        lua_pushstring(L, event->name); // L: table name
        lua_setfield(L, -2, "name"); // L: table
    }
}

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;
    // We allocate the buffer for /struct inotify_event/'s on the heap rather than on the stack in
    // order to get the maximum possible alignment for it and not resort to compiler-dependent hacks
    // like this one recommended by inotify(7):
    //     /__attribute__ ((aligned(__alignof__(struct inotify_event))))/.
    enum { NBUF = sizeof(struct inotify_event) + NAME_MAX + 2 };
    char *buf = LS_XNEW(char, NBUF);

    if (p->greet) {
        lua_State *L = funcs.call_begin(pd->userdata);
        lua_createtable(L, 0, 1); // L: table
        lua_pushstring(L, "hello"); // L: table string
        lua_setfield(L, -2, "what"); // L: table
        funcs.call_end(pd->userdata);
    }

    while (1) {
        double tmo = ls_pushed_timeout_fetch(&p->pushed_tmo, p->tmo);
        int nfds = ls_wait_input_on_fd(p->fd, tmo);

        if (nfds < 0) {
            LS_FATALF(pd, "ls_wait_input_on_fd: %s", ls_strerror_onstack(errno));
            goto error;

        } else if (nfds == 0) {
            lua_State *L = funcs.call_begin(pd->userdata);
            lua_createtable(L, 0, 1); // L: table
            lua_pushstring(L, "timeout"); // L: table string
            lua_setfield(L, -2, "what"); // L: table
            funcs.call_end(pd->userdata);

        } else {
            ssize_t r = read(p->fd, buf, NBUF);
            if (r < 0) {
                if (errno == EINTR) {
                    continue;
                }
                LS_FATALF(pd, "read: %s", ls_strerror_onstack(errno));
                goto error;
            } else if (r == 0) {
                LS_FATALF(pd, "read() from the inotify file descriptor returned 0");
                goto error;
            } else if (r == NBUF) {
                LS_FATALF(pd, "got an event with filename length > NAME_MAX+1");
                goto error;
            }
            const struct inotify_event *event;
            for (char *ptr = buf;
                 ptr < buf + r;
                 ptr += sizeof(struct inotify_event) + event->len)
            {
                event = (const struct inotify_event *) ptr;
                push_event(funcs.call_begin(pd->userdata), event);
                funcs.call_end(pd->userdata);
            }
        }
    }

error:
    free(buf);
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .run = run,
    .destroy = destroy,
};
