#include <lua.h>
#include <stdlib.h>
#include <time.h>

#include "include/barlib_v1.h"
#include "include/sayf_macros.h"

#include "libls/alloc_utils.h"
#include "libls/cstring_utils.h"
#include "libls/parse_int.h"

typedef struct {
    size_t nwidgets;
    unsigned char *widgets;
    int nevents;
} Priv;

static
void
destroy(LuastatusBarlibData *bd)
{
    Priv *p = bd->priv;
    free(p->widgets);
    free(p);
}

static
int
init(LuastatusBarlibData *bd, const char *const *opts, size_t nwidgets)
{
    Priv *p = bd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .nwidgets = nwidgets,
        .widgets = LS_XNEW0(unsigned char, nwidgets),
        .nevents = 0,
    };
    for (const char *const *s = opts; *s; ++s) {
        const char *v;
        if ((v = ls_strfollow(*s, "gen_events="))) {
            if ((p->nevents = ls_full_strtou(v)) < 0) {
                LS_FATALF(bd, "gen_events value is not a proper integer");
                goto error;
            }
        } else {
            LS_FATALF(bd, "unknown option: '%s'", *s);
            goto error;
        }
    }

    return LUASTATUS_OK;
error:
    destroy(bd);
    return LUASTATUS_ERR;
}

static
int
set(LuastatusBarlibData *bd, lua_State *L, size_t widget_idx)
{
    Priv *p = bd->priv;
    if (!lua_isnil(L, -1)) {
        LS_ERRF(bd, "got non-nil data");
        return LUASTATUS_NONFATAL_ERR;
    }
    ++p->widgets[widget_idx];
    return LUASTATUS_OK;
}

static
int
set_error(LuastatusBarlibData *bd, size_t widget_idx)
{
    Priv *p = bd->priv;
    --p->widgets[widget_idx];
    return LUASTATUS_OK;
}

static
int
event_watcher(LuastatusBarlibData *bd, LuastatusBarlibEWFuncs funcs)
{
    Priv *p = bd->priv;
    if (p->nevents && !p->nwidgets) {
        LS_FATALF(bd, "no widgets to generate events on");
        return LUASTATUS_ERR;
    }
    unsigned seed = time(NULL);
    for (int i = 0; i < p->nevents; ++i) {
        size_t widget_idx = rand_r(&seed) % p->nwidgets;
        lua_State *L = funcs.call_begin(bd->userdata, widget_idx);
        lua_pushnil(L);
        funcs.call_end(bd->userdata, widget_idx);
    }
    return LUASTATUS_NONFATAL_ERR;
}

LuastatusBarlibIface luastatus_barlib_iface_v1 = {
    .init = init,
    .set = set,
    .set_error = set_error,
    .event_watcher = event_watcher,
    .destroy = destroy,
};
