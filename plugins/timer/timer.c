#include <errno.h>
#include <lua.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"
#include "include/plugin_utils.h"

#include "libls/alloc_utils.h"
#include "libls/time_utils.h"
#include "libls/cstring_utils.h"
#include "libls/evloop_utils.h"

typedef struct {
    struct timespec period;
    char *fifo;
    LSPushedTimeout pushed_timeout;
} Priv;

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->fifo);
    ls_pushed_timeout_destroy(&p->pushed_timeout);
    free(p);
}

static
int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .period = {.tv_sec = 1},
        .fifo = NULL,
    };
    ls_pushed_timeout_init(&p->pushed_timeout);

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
void
register_funcs(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv;
    // L: table
    ls_pushed_timeout_push_luafunc(&p->pushed_timeout, L); // L: table func
    lua_setfield(L, -2, "push_period"); // L: table
}

static
void
run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    LSWakeupFifo w;
    ls_wakeup_fifo_init(&w, p->fifo, NULL);

    const char *what = "hello";

    while (1) {
        lua_State *L = funcs.call_begin(pd->userdata);
        lua_pushstring(L, what);
        funcs.call_end(pd->userdata);

        if (ls_wakeup_fifo_open(&w) < 0) {
            LS_WARNF(pd, "ls_wakeup_fifo_open: %s: %s", p->fifo, ls_strerror_onstack(errno));
        }
        int r = ls_wakeup_fifo_wait(&w, ls_pushed_timeout_fetch(&p->pushed_timeout, p->period));
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
