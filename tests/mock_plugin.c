#include <stdlib.h>
#include <limits.h>

#include <lua.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"
#include "include/plugin_utils.h"

#include "libls/alloc_utils.h"
#include "libls/algo.h"

typedef struct {
    int ncalls;
} Priv;

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p);
}

static
int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .ncalls = 0,
    };

    PU_MAYBE_VISIT_NUM_FIELD(-1, "make_calls", "'make_calls'", n,
        if (!ls_is_between_d(n, 0, INT_MAX)) {
            LS_FATALF(pd, "'make_calls' is invalid");
            goto error;
        }
        p->ncalls = n;
    );

    return LUASTATUS_OK;
error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static
void
run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;
    for (int i = 0; i < p->ncalls; ++i) {
        lua_State *L = funcs.call_begin(pd->userdata);
        lua_pushnil(L);
        funcs.call_end(pd->userdata);
    }
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
