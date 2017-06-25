#include <errno.h>
#include <stdbool.h>
#include <lua.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <sys/statvfs.h>
#include "include/plugin.h"
#include "include/sayf_macros.h"
#include "include/plugin_utils.h"
#include "libls/alloc_utils.h"
#include "libls/lua_utils.h"
#include "libls/vector.h"
#include "libls/time_utils.h"
#include "libls/errno_utils.h"
#include "libls/wakeup_fifo.h"

typedef struct {
    LS_VECTOR_OF(char *) paths;
    struct timespec period;
    char *fifo;
} Priv;

void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    for (size_t i = 0; i < p->paths.size; ++i) {
        free(p->paths.data[i]);
    }
    LS_VECTOR_FREE(p->paths);
    free(p->fifo);
    free(p);
}

int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .paths = LS_VECTOR_NEW(),
        .period = (struct timespec) {10, 0},
        .fifo = NULL,
    };

    PU_TRAVERSE_TABLE("paths",
        PU_CHECK_TYPE_AT(LS_LUA_KEY, "'paths' key", LUA_TNUMBER);
        PU_VISIT_STR_AT(LS_LUA_VALUE, "'paths' element", s,
            LS_VECTOR_PUSH(p->paths, ls_xstrdup(s));
        );
    );
    if (!p->paths.size) {
        LS_WARNF(pd, "paths are empty");
    }

    PU_MAYBE_VISIT_NUM("period", n,
        if (ls_timespec_is_invalid(p->period = ls_timespec_from_seconds(n))) {
            LS_FATALF(pd, "invalid 'period' value");
            goto error;
        }
    );

    PU_MAYBE_VISIT_STR("fifo", s,
        p->fifo = ls_xstrdup(s);
    );

    return LUASTATUS_RES_OK;

error:
    destroy(pd);
    return LUASTATUS_RES_ERR;
}

bool
push_for(LuastatusPluginData *pd, lua_State *L, const char *path)
{
    struct statvfs st;
    if (statvfs(path, &st) < 0) {
        LS_WITH_ERRSTR(s, errno,
            LS_WARNF(pd, "statvfs: %s: %s", path, s);
        );
        return false;
    }
    lua_newtable(L); // L: table
    lua_pushnumber(L, (double) st.f_frsize * st.f_blocks); // L: table n
    lua_setfield(L, -2, "total"); // L: table
    lua_pushnumber(L, (double) st.f_frsize * st.f_bfree); // L: table n
    lua_setfield(L, -2, "free"); // L: table
    lua_pushnumber(L, (double) st.f_frsize * st.f_bavail); // L: table n
    lua_setfield(L, -2, "avail"); // L: table
    return true;
}

void
run(
    LuastatusPluginData *pd,
    LuastatusPluginCallBegin call_begin,
    LuastatusPluginCallEnd call_end)
{
    Priv *p = pd->priv;
    LSWakeupFifo w;

    if (ls_wakeup_fifo_init(&w) < 0) {
        LS_WITH_ERRSTR(s, errno,
            LS_FATALF(pd, "ls_wakeup_fifo_init: %s", s);
        );
        goto error;
    }
    w.fifo = p->fifo;
    w.timeout = p->period;

    while (1) {
        // make a call
        lua_State *L = call_begin(pd->userdata);
        lua_newtable(L);
        for (size_t i = 0; i < p->paths.size; ++i) {
            const char *path = p->paths.data[i];
            if (push_for(pd, L, path)) {
                lua_setfield(L, -2, path);
            }
        }
        call_end(pd->userdata);
        // wait
        if (ls_wakeup_fifo_open(&w) < 0) {
            LS_WITH_ERRSTR(s, errno,
                LS_WARNF(pd, "ls_wakeup_fifo_open: %s: %s", p->fifo, s);
            );
        }
        if (ls_wakeup_fifo_wait(&w) < 0) {
            LS_WITH_ERRSTR(s, errno,
                LS_FATALF(pd, "ls_wakeup_fifo_wait: %s", s);
                goto error;
            );
        }
    }

error:
    ls_wakeup_fifo_destroy(&w);
}

LuastatusPluginIface luastatus_plugin_iface = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
