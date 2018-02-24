#include <lua.h>
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
#include "include/plugin_utils.h"

#include "libls/errno_utils.h"
#include "libls/vector.h"

#include "inotify_compat.h"

typedef struct {
    char *path;
    int wd;
} Watch;

typedef struct {
    int fd;
    LS_VECTOR_OF(Watch) init_watch;
    bool greet;
} Priv;

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    ls_close(p->fd);
    for (size_t i = 0; i < p->init_watch.size; ++i) {
        free(p->init_watch.data[i].path);
    }
    LS_VECTOR_FREE(p->init_watch);
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

    {0},
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
int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .fd = -1,
        .init_watch = LS_VECTOR_NEW(),
        .greet = false,
    };

    if ((p->fd = compat_inotify_init(false, true)) < 0) {
        LS_WITH_ERRSTR(s, errno,
            LS_FATALF(pd, "inotify_init: %s", s);
        );
        goto error;
    }

    PU_MAYBE_VISIT_BOOL("greet", b,
        p->greet = b;
    );

    PU_TRAVERSE_TABLE("watch",
        uint32_t mask = 0;
        const char *path;

        // Inspect the value
        PU_TRAVERSE_TABLE_AT(LS_LUA_VALUE, "'watch' value",
            PU_CHECK_TYPE_AT(LS_LUA_KEY, "key of 'watch' value", LUA_TNUMBER);
            PU_VISIT_STR_AT(LS_LUA_VALUE, "element of 'watch' value", s,
                uint32_t r = parse_input_event_name(s);
                if (!r) {
                    LS_FATALF(pd, "unknown input event type name '%s'", s);
                    goto error;
                }
                mask |= r;
            );
        );

        // Inspect the key
        PU_VISIT_LSTR_AT(LS_LUA_KEY, "'watch' key", s, ns,
            if (strlen(s) != ns) {
                LS_FATALF(pd, "'watch' key contains a NIL character");
                goto error;
            }
            path = s;
        );

        if (!mask) {
            LS_WARNF(pd, "'watch' value for key '%s' an empty table", path);
        }
        int wd = inotify_add_watch(p->fd, path, mask);
        if (wd < 0) {
            LS_WITH_ERRSTR(s, errno,
                LS_ERRF(pd, "inotify_add_watch: %s: %s", path, s);
            );
        } else {
            LS_VECTOR_PUSH(p->init_watch, ((Watch) {
                .path = ls_xstrdup(path),
                .wd = wd,
            }));
        }
    );
    if (!p->init_watch.size) {
        LS_WARNF(pd, "nothing to watch for");
    }

    return LUASTATUS_OK;

error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static
int
l_add_watch(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    uint32_t mask = 0;
    LS_LUA_TRAVERSE(L, 2) {
        // Inspect the value
        if (!lua_isnumber(L, LS_LUA_KEY)) {
            return luaL_error(L, "second argument is not an array (has a non-numeric key)");
        }
        // Inspect the key
        if (!lua_isstring(L, LS_LUA_VALUE)) {
            return luaL_error(L, "array element: string expected, found %s",
                luaL_typename(L, LS_LUA_VALUE));
        }
        size_t nevent;
        const char *event = lua_tolstring(L, LS_LUA_VALUE, &nevent);
        if (nevent != strlen(event)) {
            return luaL_error(L, "array element contains a NIL character");
        }
        uint32_t r = parse_input_event_name(event);
        if (!r) {
            return luaL_error(L, "unknown input event type name '%s'", event);
        }
        mask |= r;
    }

    LuastatusPluginData *pd = lua_touserdata(L, lua_upvalueindex(1));
    Priv *p = pd->priv;

    if (!mask) {
        LS_WARNF(pd, "add_watch: empty table passed");
    }
    int wd = inotify_add_watch(p->fd, path, mask);
    if (wd < 0) {
        LS_WITH_ERRSTR(s, errno,
            LS_ERRF(pd, "inotify_add_watch: %s: %s", path, s);
        );
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
        LS_WITH_ERRSTR(s, errno,
            LS_ERRF(pd, "inotify_rm_watch: %d: %s", wd, s);
        );
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
#define REG_CLOSURE(Func_, Name_) \
    do { \
        /* L: table */ \
        lua_pushlightuserdata(L, pd); /* L: table bd */ \
        lua_pushcclosure(L, Func_, 1); /* L: table bd Func_ */ \
        ls_lua_rawsetf(L, Name_); /* L: table */ \
    } while (0)

    REG_CLOSURE(l_add_watch, "add_watch");
    REG_CLOSURE(l_remove_watch, "remove_watch");
    REG_CLOSURE(l_get_initial_wds, "get_initial_wds");

#undef REG_CLOSURE
}

static
void
push_event(lua_State *L, const struct inotify_event *event)
{
    // L: -
    lua_newtable(L); // L: table

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
        lua_pushnil(funcs.call_begin(pd->userdata));
        funcs.call_end(pd->userdata);
    }

    char buf[sizeof(struct inotify_event) + NAME_MAX + 2]
        __attribute__ ((aligned(__alignof__(struct inotify_event))));

    const int fd = p->fd;
    while (1) {
        ssize_t r = read(fd, buf, sizeof(buf));
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            LS_WITH_ERRSTR(s, errno,
                LS_FATALF(pd, "read: %s", s);
            );
            return;
        } else if (r == 0) {
            LS_FATALF(pd, "read() from the inotify file descriptor returned 0");
            return;
        } else if ((size_t) r == sizeof(buf)) {
            LS_FATALF(pd, "got an event with filename length > NAME_MAX+1");
            return;
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

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .run = run,
    .destroy = destroy,
};
