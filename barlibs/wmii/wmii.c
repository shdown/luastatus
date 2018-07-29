#include "include/barlib_v1.h"
#include "include/sayf_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <ixp.h>

#include "libls/alloc_utils.h"
#include "libls/cstring_utils.h"

typedef struct {
    size_t nwidgets;
    unsigned char *exists;
    char barsym;
    IxpClient *client;
} Priv;

static
void
destroy(LuastatusBarlibData *bd)
{
    Priv *p = bd->priv;
    free(p->exists);
    if (p->client) {
        ixp_unmount(p->client);
    }
    free(p);
}

static
int
init(LuastatusBarlibData *bd, const char *const *opts, size_t nwidgets)
{
    Priv *p = bd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .nwidgets = nwidgets,
        .exists = LS_XNEW0(unsigned char, nwidgets),
        .barsym = 'r',
        .client = NULL,
    };

    const char *addr = NULL;
    for (const char *const *s = opts; *s; ++s) {
        const char *v;
        if ((v = ls_strfollow(*s, "address="))) {
            addr = v;
        } else if ((v = ls_strfollow(*s, "bar="))) {
            if ((v[0] != 'l' && v[0] != 'r') || v[1] != '\0') {
                LS_FATALF(bd, "invalid bar= option value: '%s' (expected 'l' or 'r')", v);
                goto error;
            }
            p->barsym = v[0];
        } else {
            LS_FATALF(bd, "unknown option '%s'", *s);
            goto error;
        }
    }

    if (!(p->client = ixp_mount(addr))) {
        LS_FATALF(bd, "ixp_mount: %s: %s", addr, ixp_errbuf());
        goto error;
    }

    return LUASTATUS_OK;

error:
    destroy(bd);
    return LUASTATUS_ERR;
}

#define NPATH 32

static inline
void
get_path(char *buf, Priv *p, size_t widget_idx)
{
    snprintf(buf, NPATH, "/%cbar/LS%03zu", p->barsym, widget_idx);
}

static
bool
set_content(LuastatusBarlibData *bd, size_t widget_idx, const char *buf, size_t nbuf)
{
    Priv *p = bd->priv;

    char path[NPATH];
    get_path(path, p, widget_idx);

    IxpCFid *f;
    if (p->exists[widget_idx]) {
        if (!(f = ixp_open(p->client, path, P9_OWRITE))) {
            LS_ERRF(bd, "ixp_open: %s: %s", path, ixp_errbuf());
            return false;
        }
    } else {
        if (!(f = ixp_create(p->client, path, 6 /* rw- */, P9_OWRITE))) {
            LS_ERRF(bd, "ixp_create: %s: %s", path, ixp_errbuf());
            return false;
        }
        p->exists[widget_idx] = 1;
    }

    bool ret = true;
    if (ixp_write(f, buf, (long) nbuf) != (long) nbuf) {
        LS_ERRF(bd, "ixp_write [%s]: %s", path, ixp_errbuf());
        ret = false;
    }
    if (ixp_close(f) != 1) {
        // only give a warning
        LS_WARNF(bd, "ixp_close [%s]: %s", path, ixp_errbuf());
    }
    return ret;
}

static
bool
clear_content(LuastatusBarlibData *bd, size_t widget_idx)
{
    Priv *p = bd->priv;

    if (p->exists[widget_idx]) {
        char path[NPATH];
        get_path(path, p, widget_idx);

        if (ixp_remove(p->client, path) != 1) {
            LS_ERRF(bd, "ixp_remove: %s: %s", path, ixp_errbuf());
            return false;
        }
    }

    return true;
}

static
int
set(LuastatusBarlibData *bd, lua_State *L, size_t widget_idx)
{
    switch (lua_type(L, -1)) {
    case LUA_TNIL:
        if (!clear_content(bd, widget_idx)) {
            return LUASTATUS_ERR;
        }
        break;
    case LUA_TSTRING:
        {
            size_t ns;
            const char *s = lua_tolstring(L, -1, &ns);
            if (!set_content(bd, widget_idx, s, ns)) {
                return LUASTATUS_ERR;
            }
        }
        break;
    default:
        LS_ERRF(bd, "expected string or nil, got %s", luaL_typename(L, -1));
        return LUASTATUS_NONFATAL_ERR;
    }
    return LUASTATUS_OK;
}

static
int
set_error(LuastatusBarlibData *bd, size_t widget_idx)
{
    const char *text = "(Error)";
    if (!set_content(bd, widget_idx, text, strlen(text))) {
        return LUASTATUS_ERR;
    }
    return LUASTATUS_OK;
}

LuastatusBarlibIface luastatus_barlib_iface_v1 = {
    .init = init,
    .set = set,
    .set_error = set_error,
    .destroy = destroy,
};
