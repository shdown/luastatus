#include <errno.h>
#include <lua.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"
#include "include/plugin_utils.h"

#include "libls/alloc_utils.h"
#include "libls/panic.h"
#include "libls/lua_utils.h"
#include "libls/time_utils.h"
#include "libls/cstring_utils.h"
#include "libls/wakeup_fifo.h"

typedef struct {
    struct timespec period;
    char *fifo;

    struct timespec pushed_period;
    pthread_spinlock_t push_lock;
} Priv;

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->fifo);
    pthread_spin_destroy(&p->push_lock);
    free(p);
}

static
int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .period = (struct timespec) {.tv_sec = 1},
        .fifo = NULL,
        .pushed_period = ls_timespec_invalid,
    };

    if (pthread_spin_init(&p->push_lock, PTHREAD_PROCESS_PRIVATE) != 0) {
        LS_PANIC("pthread_spin_init() failed, which is impossible");
    }

    PU_MAYBE_VISIT_NUM_FIELD(-1, "period", "'period'", n,
        if (ls_timespec_is_invalid(p->period = ls_timespec_from_seconds(n))) {
            LS_FATALF(pd, "invalid 'period' value");
            goto error;
        }
    );

    PU_MAYBE_VISIT_STR_FIELD(-1, "fifo", "'fifo'", s,
        p->fifo = ls_xstrdup(s);
    );

    return LUASTATUS_OK;

error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static
int
l_push_period(lua_State *L)
{
    LuastatusPluginData *pd = lua_touserdata(L, lua_upvalueindex(1));
    Priv *p = pd->priv;

    struct timespec period = ls_timespec_from_seconds(luaL_checknumber(L, 1));
    if (ls_timespec_is_invalid(period)) {
        return luaL_error(L, "invalid period");
    }

    pthread_spin_lock(&p->push_lock);
    p->pushed_period = period;
    pthread_spin_unlock(&p->push_lock);

    return 0;
}

static
void
register_funcs(LuastatusPluginData *pd, lua_State *L)
{
    // L: table
    lua_pushlightuserdata(L, pd); // L: table pd
    lua_pushcclosure(L, l_push_period, 1); // L: table pd l_push_period
    ls_lua_rawsetf(L, "push_period"); // L: table
}

static
void
run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    LSWakeupFifo w;
    ls_wakeup_fifo_init(&w, p->fifo, ls_timespec_invalid, NULL);

    const char *what = "hello";

    while (1) {
        // make a call
        lua_State *L = funcs.call_begin(pd->userdata);
        lua_pushstring(L, what);
        funcs.call_end(pd->userdata);
        // set timeout
        pthread_spin_lock(&p->push_lock);
        if (ls_timespec_is_invalid(p->pushed_period)) {
            w.timeout = p->period;
        } else {
            w.timeout = p->pushed_period;
            p->pushed_period = ls_timespec_invalid;
        }
        pthread_spin_unlock(&p->push_lock);
        // wait
        if (ls_wakeup_fifo_open(&w) < 0) {
            LS_WARNF(pd, "ls_wakeup_fifo_open: %s: %s", p->fifo, ls_strerror_onstack(errno));
        }
        int r = ls_wakeup_fifo_wait(&w);
        if (r < 0) {
            LS_FATALF(pd, "ls_wakeup_fifo_wait: %s", ls_strerror_onstack(errno));
            goto error;
        } else if (r == 0) {
            what = "timeout";
        } else {
            what = "fifo";
        }
    }

error:
    ls_wakeup_fifo_destroy(&w);
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .run = run,
    .destroy = destroy,
};
