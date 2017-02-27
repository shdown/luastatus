#include <errno.h>
#include <fcntl.h>
#include <lua.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <sys/statvfs.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include "include/plugin.h"
#include "include/plugin_logf_macros.h"
#include "include/plugin_utils.h"
#include "libls/alloc_utils.h"
#include "libls/lua_utils.h"
#include "libls/osdep.h"
#include "libls/vector.h"
#include "libls/time_utils.h"
#include "libls/errno_utils.h"
#include "libls/wakeup_fifo.h"

typedef struct {
    struct timespec period;
    char *fifo;
} Priv;

void
priv_destroy(Priv *p)
{
    free(p->fifo);
}

LuastatusPluginInitResult
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .period = (struct timespec) {1, 0},
        .fifo = NULL,
    };

    PU_MAYBE_VISIT_NUM("period", n,
        if (ls_timespec_is_invalid(p->period = ls_timespec_from_seconds(n))) {
            LUASTATUS_FATALF(pd, "invalid 'period' value");
            goto error;
        }
    );

    PU_MAYBE_VISIT_STR("fifo", s,
        p->fifo = ls_xstrdup(s);
    );

    return LUASTATUS_PLUGIN_INIT_RESULT_OK;

error:
    priv_destroy(p);
    free(p);
    return LUASTATUS_PLUGIN_INIT_RESULT_ERR;
}

void
run(
    LuastatusPluginData *pd,
    LuastatusPluginCallBegin call_begin,
    LuastatusPluginCallEnd call_end)
{
    Priv *p = pd->priv;

    LSWakeupFifo w;
    ls_wakeup_fifo_init(&w);

    sigset_t allsigs;
    if (sigfillset(&allsigs) < 0) {
        LS_WITH_ERRSTR(s, errno,
            LUASTATUS_FATALF(pd, "sigfillset: %s", s);
        );
        goto error;
    }
    ls_wakeup_fifo_setup(&w, p->fifo, &p->period, &allsigs);

    const char *what = "hello";

    while (1) {
        // make a call
        lua_State *L = call_begin(pd->userdata);
        lua_pushstring(L, what);
        call_end(pd->userdata);
        // wait
        if (ls_wakeup_fifo_open(&w) < 0) {
            LS_WITH_ERRSTR(s, errno,
                LUASTATUS_WARNF(pd, "open: %s: %s", p->fifo, s);
            );
        }
        int r = ls_wakeup_fifo_pselect(&w);
        if (r < 0) {
            LS_WITH_ERRSTR(s, errno,
                LUASTATUS_FATALF(pd, "pselect: %s", s);
            );
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

void
destroy(LuastatusPluginData *pd)
{
    priv_destroy(pd->priv);
    free(pd->priv);
}

LuastatusPluginIface luastatus_plugin_iface = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
