#include <lua.h>
#include <lauxlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/select.h>
#include <signal.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"
#include "include/plugin_utils.h"

#include "libls/lua_utils.h"
#include "libls/alloc_utils.h"
#include "libls/cstring_utils.h"
#include "libls/vector.h"
#include "libls/time_utils.h"
#include "libls/sig_utils.h"
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
    struct timespec timeout;
    LSPushedTimeout pushed_timeout;
} Priv;

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    close(p->fd);
    for (size_t i = 0; i < p->init_watch.size; ++i) {
        free(p->init_watch.data[i].path);
    }
    LS_VECTOR_FREE(p->init_watch);
    ls_pushed_timeout_destroy(&p->pushed_timeout);
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

// returns 0 on failure
static inline
uint32_t
parse_input_event_name(const char *name)
{
    for (const EventType *et = EVENT_TYPES; et->name; ++et) {
        if (et->in && strcmp(et->name, name) == 0) {
            return et->mask;
        }
    }
    return 0;
}

static
bool
process_path(lua_State *L, int pos, char const **ppath, char *errbuf, size_t nerrbuf)
{
    if (!lua_isstring(L, pos)) {
        snprintf(errbuf, nerrbuf, "expected string, found %s", luaL_typename(L, pos));
        return false;
    }
    size_t npath;
    const char *path = lua_tolstring(L, pos, &npath);
    if (strlen(path) != npath) {
        snprintf(errbuf, nerrbuf, "contains a NUL character");
        return false;
    }
    *ppath = path;
    return true;
}

static
bool
process_evnames(lua_State *L, int pos, uint32_t *pmask, char *errbuf, size_t nerrbuf)
{
    if (!lua_istable(L, pos)) {
        snprintf(errbuf, nerrbuf, "expected table, found %s", luaL_typename(L, pos));
        return false;
    }
    uint32_t mask = 0;
    LS_LUA_TRAVERSE(L, pos) {
        if (!lua_isstring(L, LS_LUA_VALUE)) {
            snprintf(errbuf, nerrbuf, "value: expected string, found %s",
                     luaL_typename(L, LS_LUA_VALUE));
            return false;
        }
        const char *evname = lua_tostring(L, LS_LUA_VALUE);
        uint32_t r = parse_input_event_name(evname);
        if (!r) {
            snprintf(errbuf, nerrbuf, "unknown input event name '%s'", evname);
            return false;
        }
        mask |= r;
    }
    *pmask = mask;
    return true;
}

static
int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .fd = -1,
        .init_watch = LS_VECTOR_NEW(),
        .greet = false,
        .timeout = ls_timespec_invalid,
    };
    ls_pushed_timeout_init(&p->pushed_timeout);

    if ((p->fd = compat_inotify_init(false, true)) < 0) {
        LS_FATALF(pd, "inotify_init: %s", ls_strerror_onstack(errno));
        goto error;
    }

    PU_MAYBE_VISIT_BOOL_FIELD(-1, "greet", "'greet'", b,
        p->greet = b;
    );

    PU_MAYBE_VISIT_NUM_FIELD(-1, "timeout", "'timeout'", n,
        if (ls_timespec_is_invalid(p->timeout = ls_timespec_from_seconds(n)) && n >= 0) {
            LS_FATALF(pd, "'timeout' is invalid");
            goto error;
        }
    );

    char err[256];
    PU_VISIT_TABLE_FIELD(-1, "watch", "'watch'",
        const char *path;
        uint32_t mask;

        if (!process_path(L, LS_LUA_KEY, &path, err, sizeof(err))) {
            LS_FATALF(pd, "'watch' key: %s", err);
            goto error;
        }
        if (!process_evnames(L, LS_LUA_VALUE, &mask, err, sizeof(err))) {
            LS_FATALF(pd, "'watch' value: %s", err);
            goto error;
        }

        int wd = inotify_add_watch(p->fd, path, mask);
        if (wd < 0) {
            LS_ERRF(pd, "inotify_add_watch: %s: %s", path, ls_strerror_onstack(errno));
        } else {
            LS_VECTOR_PUSH(p->init_watch, ((Watch) {
                .path = ls_xstrdup(path),
                .wd = wd,
            }));
        }
    );

    return LUASTATUS_OK;

error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static
int
l_add_watch(lua_State *L)
{
    if (lua_gettop(L) != 2) {
        return luaL_error(L, "expected exactly 2 arguments");
    }
    const char *path;
    uint32_t mask;

    char err[256];
    if (!process_path(L, 1, &path, err, sizeof(err))) {
        return luaL_error(L, "first argument: %s", err);
    }
    if (!process_evnames(L, 2, &mask, err, sizeof(err))) {
        return luaL_error(L, "second argument: %s", err);
    }

    LuastatusPluginData *pd = lua_touserdata(L, lua_upvalueindex(1));
    Priv *p = pd->priv;

    if (!mask) {
        LS_WARNF(pd, "add_watch: empty table passed");
    }
    int wd = inotify_add_watch(p->fd, path, mask);
    if (wd < 0) {
        LS_ERRF(pd, "inotify_add_watch: %s: %s", path, ls_strerror_onstack(errno));
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, wd);
    }
    return 1;
}

static
int
l_remove_watch(lua_State *L)
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

static
int
l_get_initial_wds(lua_State *L)
{
    LuastatusPluginData *pd = lua_touserdata(L, lua_upvalueindex(1));
    Priv *p = pd->priv;

    lua_createtable(L, 0, p->init_watch.size); // L: table
    for (size_t i = 0; i < p->init_watch.size; ++i) {
        lua_pushinteger(L, p->init_watch.data[i].wd); // L: table wd
        lua_setfield(L, -2, p->init_watch.data[i].path); // L: table
    }
    return 1;
}

static
void
register_funcs(LuastatusPluginData *pd, lua_State *L)
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
    ls_pushed_timeout_push_luafunc(&p->pushed_timeout, L); // L: table func
    lua_setfield(L, -2, "push_timeout"); // L: table
}

static
void
push_event(lua_State *L, const struct inotify_event *event)
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

static
void
run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    if (p->greet) {
        lua_State *L = funcs.call_begin(pd->userdata);
        lua_createtable(L, 0, 1); // L: table
        lua_pushstring(L, "hello"); // L: table string
        lua_setfield(L, -2, "what"); // L: table
        funcs.call_end(pd->userdata);
    }

    // This /__attribute__((aligned(__alignof__(struct inotify_event))))/ thing is in inotify's
    // man page.
    // Since inotify is Linux-specific, and you need gcc to build the Linux kernel, it's probably
    // justified that gcc (or at least a GNU C-compatible compiler) is also needed to build this
    // plugin.
    char buf[sizeof(struct inotify_event) + NAME_MAX + 2]
        __attribute__((aligned(__alignof__(struct inotify_event))));

    fd_set fds;
    FD_ZERO(&fds);

    sigset_t allsigs;
    ls_xsigfillset(&allsigs);

    while (1) {
        struct timespec timeout = ls_pushed_timeout_fetch(&p->pushed_timeout, p->timeout);
        FD_SET(p->fd, &fds);
        const int nfds = pselect(
            p->fd + 1,
            &fds, NULL, NULL,
            ls_timespec_is_invalid(timeout) ? NULL : &timeout,
            &allsigs);

        if (nfds < 0) {
            LS_FATALF(pd, "pselect: %s", ls_strerror_onstack(errno));
            goto error;
        } else if (nfds == 0) {
            lua_State *L = funcs.call_begin(pd->userdata);
            lua_createtable(L, 0, 1); // L: table
            lua_pushstring(L, "timeout"); // L: table string
            lua_setfield(L, -2, "what"); // L: table
            funcs.call_end(pd->userdata);
            continue;
        }

        ssize_t r = read(p->fd, buf, sizeof(buf));
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            LS_FATALF(pd, "read: %s", ls_strerror_onstack(errno));
            goto error;
        } else if (r == 0) {
            LS_FATALF(pd, "read() from the inotify file descriptor returned 0");
            goto error;
        } else if ((size_t) r == sizeof(buf)) {
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

error:
    return;
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .run = run,
    .destroy = destroy,
};
