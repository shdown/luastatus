#include "include/barlib_v1.h"
#include "include/sayf_macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <lua.h>
#include <lauxlib.h>
#include <errno.h>

#include <ixp.h>

#include "libls/alloc_utils.h"
#include "libls/cstring_utils.h"
#include "libls/vector.h"
#include "libls/string_.h"
#include "libls/getenv_r.h"
#include "libls/io_utils.h"
#include "libls/parse_int.h"
#include "libls/lua_utils.h"

#define NDIR         10
#define DIR_PRI_FMT  "/%cbar"

#define NFILE        20
#define FILE_PRI_FMT "LS%03zu_%03u"
#define FILE_SCN_FMT "LS%zu_%u"

#define NPATH        (NDIR + NFILE + 1)
#define PATH_PRI_FMT DIR_PRI_FMT "/" FILE_PRI_FMT

typedef struct {
    size_t nwidgets;

    // /nfiles[i]/ is the largest /n/ such that the file named
    //     "/?bar/LS###_@@@",
    // where "?" is the value of /barsym/, "###" is zero-padded /i/, and "@@@" is zero-padded /n/,
    // is expected to exist.
    unsigned *nfiles;

    // Bar side symbol: either 'l' or 'r'.
    char barsym;

    // Length of the "preface" prefix in /buf/.
    size_t npreface;

    // Widget content buffer.
    LSString buf;

    IxpClient *client;
} Priv;

static
void
destroy(LuastatusBarlibData *bd)
{
    Priv *p = bd->priv;
    free(p->nfiles);
    LS_VECTOR_FREE(p->buf);
    if (p->client) {
        ixp_unmount(p->client);
    }
    free(p);
}

static
bool
remove_leftovers(LuastatusBarlibData *bd)
{
    Priv *p = bd->priv;

    char dir[NDIR];
    snprintf(dir, sizeof(dir), DIR_PRI_FMT, p->barsym);

    IxpCFid *f = ixp_open(p->client, dir, P9_OREAD | P9_OCEXEC);
    if (!f) {
        LS_FATALF(bd, "ixp_open: %s: %s", dir, ixp_errbuf());
        return false;
    }

    char *buf = LS_XNEW(char, f->iounit);
    long nread;
    while ((nread = ixp_read(f, buf, f->iounit)) > 0) {
        IxpMsg m = ixp_message(buf, nread, MsgUnpack);
        while (m.pos < m.end) {
            IxpStat s;
            ixp_pstat(&m, &s);

            size_t widget_idx;
            unsigned file_idx;
            if (sscanf(s.name, FILE_SCN_FMT, &widget_idx, &file_idx) == 2) {
                char path[NPATH];
                snprintf(path, sizeof(path), PATH_PRI_FMT, p->barsym, widget_idx, file_idx);
                if (ixp_remove(p->client, path) != 1) {
                    LS_WARNF(bd, "ixp_remove: %s: %s", path, ixp_errbuf());
                }
            }

            ixp_freestat(&s);
        }
    }

    bool ret = true;
    if (nread < 0) {
        ret = false;
        LS_FATALF(bd, "ixp_read [%s]: %s", dir, ixp_errbuf());
    }
    free(buf);
    ixp_close(f);
    return ret;
}

static
int
init(LuastatusBarlibData *bd, const char *const *opts, size_t nwidgets)
{
    Priv *p = bd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .nwidgets = nwidgets,
        .nfiles = LS_XNEW0(unsigned, nwidgets),
        .barsym = 'r',
        .npreface = 0,
        .buf = LS_VECTOR_NEW(),
        .client = NULL,
    };

    {
        void **ptr = bd->map_get(bd->userdata, "flag:library_used:ixp");
        if (*ptr) {
            LS_FATALF(bd, "another entity is already using thread-unsafe library libixp");
            goto error;
        }
        *ptr = "yes";
    }

    if (nwidgets > 1000) {
        LS_FATALF(bd, "too many widgets (more than 1000)");
        goto error;
    }

    const char *addr = ls_getenv_r("WMII_ADDRESS");
    bool wmii3 = false;

    for (const char *const *s = opts; *s; ++s) {
        const char *v;
        if ((v = ls_strfollow(*s, "address="))) {
            addr = v;
        } else if ((v = ls_strfollow(*s, "bar="))) {
            if (strcmp(v, "l") != 0 && strcmp(v, "r") != 0) {
                LS_FATALF(bd, "invalid bar value: '%s' (expected 'l' or 'r')", v);
                goto error;
            }
            p->barsym = v[0];
        } else if (strcmp(*s, "wmii3") == 0) {
            wmii3 = true;
        } else if ((v = ls_strfollow(*s, "wmii3_addline="))) {
            wmii3 = true;
            ls_string_append_s(&p->buf, v);
            ls_string_append_c(&p->buf, '\n');
        } else {
            LS_FATALF(bd, "unknown option '%s'", *s);
            goto error;
        }
    }

    if (wmii3) {
        ls_string_append_s(&p->buf, "label ");
    }
    p->npreface = p->buf.size;

    if (addr) {
        if (!(p->client = ixp_mount(addr))) {
            LS_FATALF(bd, "ixp_mount: %s: %s", addr, ixp_errbuf());
            goto error;
        }
    } else {
        if (!(p->client = ixp_nsmount("wmii"))) {
            LS_FATALF(bd, "ixp_nsmount: wmii: %s", ixp_errbuf());
            goto error;
        }
    }
    if (!remove_leftovers(bd)) {
        goto error;
    }

    return LUASTATUS_OK;

error:
    destroy(bd);
    return LUASTATUS_ERR;
}

// Assuming /*counter/ is the number of files insofar written for the widget with index
// /widget_idx/, writes the next file with the content given by /buf/ and /nbuf/, incrementing
// /*counter/.
static
bool
append_segment(
        LuastatusBarlibData *bd,
        size_t widget_idx,
        unsigned *counter,
        const char *buf,
        size_t nbuf)
{
    Priv *p = bd->priv;

    if (*counter >= 1000) {
        LS_WARNF(bd, "too many segments for a widget (more than 1000)");
        return true;
    }
    char path[NPATH];
    snprintf(path, sizeof(path), PATH_PRI_FMT, p->barsym, widget_idx, *counter);
    ++*counter;

    IxpCFid *f = ixp_create(p->client, path, 6 /* rw- */, P9_OWRITE | P9_OCEXEC);
    if (!f) {
        LS_ERRF(bd, "ixp_create: %s: %s", path, ixp_errbuf());
        return false;
    }

    p->buf.size = p->npreface;
    ls_string_append_b(&p->buf, buf, nbuf);
    ls_string_append_c(&p->buf, '\n');

    bool ret = true;
    if (ixp_write(f, p->buf.data, p->buf.size) != (long) p->buf.size) {
        ret = false;
        LS_ERRF(bd, "ixp_write [%s]: %s", path, ixp_errbuf());
        LS_INFOF(bd, "try passing 'wmii3' option to this barlib, especially if getting 'bad value' "
                     "error");
    }
    ixp_close(f);
    return ret;
}

// Removes files corresponding to blocks from /counter/ to /p->nfiles[widget_idx]/ and updates
// /p->nfiles[widget_idx]/ on success.
//
// If /false/ is returned, the state of the files is unspecified and /p->nfiles/ cannot be further
// relied on.
static
bool
finalize(LuastatusBarlibData *bd, size_t widget_idx, unsigned counter)
{
    Priv *p = bd->priv;
    for (unsigned i = counter; i < p->nfiles[widget_idx]; ++i) {
        char path[NPATH];
        snprintf(path, sizeof(path), PATH_PRI_FMT, p->barsym, widget_idx, i);

        if (ixp_remove(p->client, path) != 1) {
            LS_ERRF(bd, "ixp_remove: %s: %s", path, ixp_errbuf());
            return false;
        }
    }
    p->nfiles[widget_idx] = counter;
    return true;
}

static
int
set(LuastatusBarlibData *bd, lua_State *L, size_t widget_idx)
{
    unsigned counter = 0;

    switch (lua_type(L, -1)) {
    case LUA_TNIL:
        break;

    case LUA_TSTRING:
        {
            size_t ns;
            const char *s = lua_tolstring(L, -1, &ns);
            if (!append_segment(bd, widget_idx, &counter, s, ns)) {
                return LUASTATUS_ERR;
            }
        }
        break;

    case LUA_TTABLE:
        LS_LUA_TRAVERSE(L, -1) {
            if (lua_isnil(L, LS_LUA_VALUE)) {
                continue;
            }
            if (!lua_isstring(L, LS_LUA_VALUE)) {
                LS_ERRF(bd, "table element: expected string, got %s", luaL_typename(L, -1));
                return LUASTATUS_NONFATAL_ERR;
            }
            size_t ns;
            const char *s = lua_tolstring(L, LS_LUA_VALUE, &ns);
            if (!append_segment(bd, widget_idx, &counter, s, ns)) {
                return LUASTATUS_ERR;
            }
        }
        break;

    default:
        LS_ERRF(bd, "expected string or nil, got %s", luaL_typename(L, -1));
        return LUASTATUS_NONFATAL_ERR;
    }

    if (!finalize(bd, widget_idx, counter)) {
        return LUASTATUS_ERR;
    }
    return LUASTATUS_OK;
}

static
int
set_error(LuastatusBarlibData *bd, size_t widget_idx)
{
    const char *text = "(Error)";
    unsigned counter = 0;
    if (!append_segment(bd, widget_idx, &counter, text, strlen(text))) {
        return LUASTATUS_ERR;
    }
    if (!finalize(bd, widget_idx, counter)) {
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
