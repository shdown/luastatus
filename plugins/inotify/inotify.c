#include <lua.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
#include <sys/inotify.h>
#include "include/plugin.h"
#include "include/plugin_logf_macros.h"
#include "include/plugin_utils.h"
#include "libls/alloc_utils.h"
#include "libls/vector.h"
#include "libls/osdep.h"
#include "libls/errno_utils.h"
#include "libls/cstring_utils.h"
#include "inotify_compat.h"

typedef struct {
    int wd;
    char *path;
} Watch;

typedef struct {
    int fd;
    LS_VECTOR_OF(Watch) watches;
} Priv;

void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    ls_close(p->fd);
    for (size_t i = 0; i < p->watches.size; ++i) {
        free(p->watches.data[i].path);
    }
    LS_VECTOR_FREE(p->watches);
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

    {0,                false, false, NULL},
};

// returns 0 on failure
static inline
uint32_t
parse_mask(LuastatusPluginData *pd, const char *buf)
{
    uint32_t mask = 0;
    while (1) {
        const char *v = NULL;
        for (const EventType *et = EVENT_TYPES; et->name; ++et) {
            if (et->in && (v = ls_strfollow(buf, et->name))) {
                if (*v == '\0' || *v == ',') {
                    mask |= et->mask;
                    break;
                } else {
                    v = NULL;
                }
            }
        }
        if (!v) {
            LUASTATUS_FATALF(pd, "can't parse mask segment starting from: '%s'", buf);
            return 0;
        }
        if (*v == '\0') {
            break;
        } else {
            buf = v + 1;
        }
    }
    return mask;
}

LuastatusPluginInitResult
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .fd = -1,
        .watches = LS_VECTOR_NEW(),
    };

    if ((p->fd = compat_inotify_init(false, true)) < 0) {
        LS_WITH_ERRSTR(s, errno,
            LUASTATUS_FATALF(pd, "inotify_init: %s", s);
        );
        goto error;
    }

    PU_TRAVERSE_TABLE("watch",
        uint32_t mask;
        PU_VISIT_STR_AT(LS_LUA_TRAVERSE_VALUE, "'watch' value", s,
            if (!(mask = parse_mask(pd, s))) {
                goto error;
            }
        );
        PU_VISIT_LSTR_AT(LS_LUA_TRAVERSE_KEY, "'watch' key", path, npath,
            if (strlen(path) != npath) {
                LUASTATUS_FATALF(pd, "'watch' key contains a NIL character");
                goto error;
            }
            int wd = inotify_add_watch(p->fd, path, mask);
            if (wd < 0) {
                LS_WITH_ERRSTR(s, errno,
                    LUASTATUS_FATALF(pd, "inotify_add_watch: %s: %s", path, s);
                );
                goto error;
            }
            LS_VECTOR_PUSH(p->watches, ((Watch) {
                .wd = wd,
                .path = ls_xstrdup(path),
            }));
        );
    );
    if (!p->watches.size) {
        LUASTATUS_WARNF(pd, "nothing to watch");
    }

    return LUASTATUS_PLUGIN_INIT_RESULT_OK;

error:
    destroy(pd);
    return LUASTATUS_PLUGIN_INIT_RESULT_ERR;
}

static
void
push_event(lua_State *L, const struct inotify_event *event, const char *path)
{
    // L: -
    lua_newtable(L); // L: table

    lua_pushstring(L, path); // L: table path
    lua_setfield(L, -2, "path"); // L: table

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

void
run(
    LuastatusPluginData *pd,
    LuastatusPluginCallBegin call_begin,
    LuastatusPluginCallEnd call_end)
{
    Priv *p = pd->priv;

#if defined(NAME_MAX) && NAME_MAX <= 8192
    char buf[sizeof(struct inotify_event) + NAME_MAX + 1]
#else
    char buf[sizeof(struct inotify_add_watch) + 8192]
#endif
        __attribute__ ((aligned(__alignof__(struct inotify_event))));

    const int fd = p->fd;
    while (1) {
        ssize_t r = read(fd, buf, sizeof(buf));
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            LS_WITH_ERRSTR(s, errno,
                LUASTATUS_FATALF(pd, "read: %s", s);
            );
            break;
        } else if (r == 0) {
            LUASTATUS_FATALF(pd, "read() returned 0 (no more events?)");
            break;
        }
        const struct inotify_event *event;
        for (char *ptr = buf;
             ptr < buf + r;
             ptr += sizeof(struct inotify_event) + event->len)
        {
            event = (const struct inotify_event *) ptr;
            const char *path = NULL;
            for (size_t i = 0; i < p->watches.size; ++i) {
                if (p->watches.data[i].wd == event->wd) {
                    path = p->watches.data[i].path;
                    break;
                }
            }
            if (path) {
                push_event(call_begin(pd->userdata), event, path);
                call_end(pd->userdata);
            } else {
                LUASTATUS_WARNF(pd, "event with unknown watch descriptor has been read");
            }
        }
    }
}

LuastatusPluginIface luastatus_plugin_iface = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
