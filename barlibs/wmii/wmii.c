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

// Size of a buffer with (zero-terminated) widget path.
#define NPATH 32

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

    // * The value of "address" option, if passed, or;
    // * the value of "WMII_ADDRESS" environment variable, if exists, or;
    // * /NULL/.
    char *addr;

    IxpClient *client;
} Priv;

static inline
void
get_path(char *buf, Priv *p, size_t widget_idx, unsigned file_idx)
{
    snprintf(buf, NPATH, "/%cbar/LS%03zu_%03u", p->barsym, widget_idx, file_idx);
}

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
    free(p->addr);
    free(p);
}

static
IxpClient *
connect(LuastatusBarlibData *bd)
{
    Priv *p = bd->priv;
    IxpClient *c;
    if (p->addr) {
        if (!(c = ixp_mount(p->addr))) {
            LS_FATALF(bd, "ixp_mount: %s: %s", p->addr, ixp_errbuf());
        }
    } else {
        if (!(c = ixp_nsmount("wmii"))) {
            LS_FATALF(bd, "ixp_nsmount: wmii: %s", ixp_errbuf());
        }
    }
    return c;
}

static
bool
remove_leftovers(LuastatusBarlibData *bd)
{
    Priv *p = bd->priv;

    char dirpath[NPATH];
    snprintf(dirpath, sizeof(dirpath), "/%cbar", p->barsym);
    IxpCFid *f = ixp_open(p->client, dirpath, P9_OREAD | P9_OCEXEC);
    if (!f) {
        LS_FATALF(bd, "ixp_open: %s: %s", dirpath, ixp_errbuf());
        return false;
    }

    char *buf = ls_xmalloc(f->iounit, 1);
    long nread;
    while ((nread = ixp_read(f, buf, f->iounit)) > 0) {
        IxpMsg m = ixp_message(buf, nread, MsgUnpack);
        while (m.pos < m.end) {
            IxpStat s;
            ixp_pstat(&m, &s);

            size_t widget_idx;
            unsigned file_idx;
            if (sscanf(s.name, "LS%zu_%u", &widget_idx, &file_idx) == 2) {
                char path[NPATH];
                get_path(path, p, widget_idx, file_idx);
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
        LS_FATALF(bd, "ixp_read [%s]: %s", dirpath, ixp_errbuf());
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
        .addr = NULL,
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

    p->addr = addr ? ls_xstrdup(addr) : NULL;

    if (!(p->client = connect(bd))) {
        goto error;
    }
    if (!remove_leftovers(bd)) {
        goto error;
    }

    return LUASTATUS_OK;

error:
    destroy(bd);
    return LUASTATUS_ERR;
}

// Passing /file_idx/ > /p->nfiles[widget_idx]/ is not allowed.
static
bool
set_content(
        LuastatusBarlibData *bd,
        size_t widget_idx,
        unsigned file_idx,
        const char *buf,
        size_t nbuf)
{
    Priv *p = bd->priv;

    char path[NPATH];
    get_path(path, p, widget_idx, file_idx);

    IxpCFid *f = ixp_create(p->client, path, 6 /* rw- */, P9_OWRITE | P9_OCEXEC);
    if (!f) {
        LS_ERRF(bd, "ixp_create: %s: %s", path, ixp_errbuf());
        return false;
    }

    if (file_idx >= p->nfiles[widget_idx]) {
        assert(file_idx == p->nfiles[widget_idx]);
        p->nfiles[widget_idx] = file_idx + 1;
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

// Removes files corresponding to blocks /new_nfiles/ to /p->nfiles[widget_idx]/ and updates
// /p->nfiles[widget_idx]/ on success.
//
// If /false/ is returned, the state of the files is unspecified and /p->nfiles/ cannot be further
// relied on.
static
bool
clean_up(LuastatusBarlibData *bd, size_t widget_idx, unsigned new_nfiles)
{
    Priv *p = bd->priv;
    for (unsigned i = new_nfiles; i < p->nfiles[widget_idx]; ++i) {
        char path[NPATH];
        get_path(path, p, widget_idx, i);

        if (ixp_remove(p->client, path) != 1) {
            LS_ERRF(bd, "ixp_remove: %s: %s", path, ixp_errbuf());
            return false;
        }
    }
    p->nfiles[widget_idx] = new_nfiles;
    return true;
}

static
int
set(LuastatusBarlibData *bd, lua_State *L, size_t widget_idx)
{
    switch (lua_type(L, -1)) {
    case LUA_TNIL:
        if (!clean_up(bd, widget_idx, 0)) {
            return LUASTATUS_ERR;
        }
        break;

    case LUA_TSTRING:
        {
            size_t ns;
            const char *s = lua_tolstring(L, -1, &ns);
            if (!set_content(bd, widget_idx, 0, s, ns)) {
                return LUASTATUS_ERR;
            }
            if (!clean_up(bd, widget_idx, 1)) {
                return LUASTATUS_ERR;
            }
        }
        break;

    case LUA_TTABLE:
        {
            unsigned n = 0;
            LS_LUA_TRAVERSE(L, -1) {
                if (n == 1000) {
                    LS_ERRF(bd, "table: too many elements (more than 1000)");
                    return LUASTATUS_NONFATAL_ERR;
                }
                if (lua_isnil(L, LS_LUA_VALUE)) {
                    continue;
                }
                if (!lua_isstring(L, LS_LUA_VALUE)) {
                    LS_ERRF(bd, "table element: expected string, got %s", luaL_typename(L, -1));
                    return LUASTATUS_NONFATAL_ERR;
                }
                size_t ns;
                const char *s = lua_tolstring(L, LS_LUA_VALUE, &ns);
                if (!set_content(bd, widget_idx, n, s, ns)) {
                    return LUASTATUS_ERR;
                }
                ++n;
            }
            if (!clean_up(bd, widget_idx, n)) {
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
    if (!set_content(bd, widget_idx, 0, text, strlen(text))) {
        return LUASTATUS_ERR;
    }
    if (!clean_up(bd, widget_idx, 1)) {
        return LUASTATUS_ERR;
    }
    return LUASTATUS_OK;
}

static
int
event_watcher(LuastatusBarlibData *bd, LuastatusBarlibEWFuncs funcs)
{
    IxpClient *c = connect(bd);
    if (!c) {
        return LUASTATUS_ERR;
    }

    const char *event_file = "/event";
    Priv *p = bd->priv;

    IxpCFid *f = ixp_open(c, event_file, P9_OREAD | P9_OCEXEC);
    if (!f) {
        LS_FATALF(bd, "ixp_open: %s: %s", event_file, ixp_errbuf());
        return LUASTATUS_ERR;
    }

    char *line = ls_xmalloc(f->iounit + 1, 1);
    long nread;
    while ((nread = ixp_read(f, line, f->iounit)) > 0) {
        line[nread] = '\0';

        char event[128];
        int button;
        size_t widget_idx;
        unsigned file_idx;
        if (sscanf(line, "%127s %d LS%zu_%u\n", event, &button, &widget_idx, &file_idx) != 4) {
            continue;
        }
        if (widget_idx >= p->nwidgets) {
            LS_WARNF(bd, "event on widget with invalid index %zu reported", widget_idx);
            continue;
        }
        const char *evdetail = ls_strfollow(event, p->barsym == 'l' ? "LeftBar" : "RightBar");
        if (!evdetail) {
            continue;
        }

        lua_State *L = funcs.call_begin(bd->userdata, widget_idx); // L: ?
        lua_createtable(L, 0, 3); // L: ? table
        lua_pushstring(L, evdetail); // L: ? table string
        lua_setfield(L, -2, "event"); // L: ? table
        lua_pushinteger(L, button); // L: ? table integer
        lua_setfield(L, -2, "button"); // L: ? table
        lua_pushinteger(L, file_idx); // L: ? table integer
        lua_setfield(L, -2, "segment"); // L: ? table
        funcs.call_end(bd->userdata, widget_idx);
    }

    LS_FATALF(bd, "ixp_read from %s failed", event_file);
    free(line);
    ixp_close(f);
    ixp_unmount(c);
    return LUASTATUS_ERR;
}

LuastatusBarlibIface luastatus_barlib_iface_v1 = {
    .init = init,
    .set = set,
    .set_error = set_error,
    .event_watcher = event_watcher,
    .destroy = destroy,
};
