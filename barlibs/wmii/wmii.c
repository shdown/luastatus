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
#include "libls/errno_utils.h"
#include "libls/vector.h"
#include "libls/string_.h"
#include "libls/getenv_r.h"
#include "libls/io_utils.h"
#include "libls/parse_int.h"

static const char *wmii_namespace = "wmii";

typedef struct {
    // Number of widgets.
    size_t nwidgets;

    // /exists[i]/ is 1 if the widget content file
    //     "/?bar/LS###", where "?" is the value of /barsym/ and "###" is zero-padded /i/,
    // is expected to exist, and 0 otherwise.
    unsigned char *exists;

    // Bar side symbol: either 'l' or 'r'.
    char barsym;

    // Length of the "preface" prefix in /buf/.
    size_t npreface;

    // Widget content buffer.
    LSString buf;

    // /fdopen/'ed /event_fd/, if passed; otherwise /NULL/.
    FILE *in;

    // libixp client handle.
    IxpClient *client;
} Priv;

static
void
destroy(LuastatusBarlibData *bd)
{
    Priv *p = bd->priv;
    free(p->exists);
    LS_VECTOR_FREE(p->buf);
    if (p->in) {
        fclose(p->in);
    }
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
        .npreface = 0,
        .buf = LS_VECTOR_NEW(),
        .in = NULL,
        .client = NULL,
    };

    const char *addr = ls_getenv_r("WMII_ADDRESS");
    bool wmii3 = false;
    int in_fd = -1;

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
        } else if ((v = ls_strfollow(*s, "event_fd="))) {
            in_fd = ls_full_parse_uint_s(v);
            if (in_fd < 0) {
                LS_FATALF(bd, "event_fd value is not a valid unsigned integer");
                goto error;
            }
            if (in_fd < 3) {
                LS_FATALF(bd, "event_fd < 3");
                goto error;
            }
        } else {
            LS_FATALF(bd, "unknown option '%s'", *s);
            goto error;
        }
    }

    if (wmii3) {
        ls_string_append_s(&p->buf, "label ");
    }
    p->npreface = p->buf.size;

    if (in_fd >= 0) {
        if (!(p->in = fdopen(in_fd, "r"))) {
            LS_WITH_ERRSTR(s, errno,
                LS_FATALF(bd, "cannot fdopen %d: %s", in_fd, s);
            );
            goto error;
        }
        if (ls_make_cloexec(in_fd) < 0) {
            LS_WITH_ERRSTR(s, errno,
                LS_FATALF(bd, "cannot make file descriptor %d CLOEXEC: %s", in_fd, s);
            );
            goto error;
        }
    }

    if (addr) {
        if (!(p->client = ixp_mount(addr))) {
            LS_FATALF(bd, "ixp_mount: %s: %s", addr, ixp_errbuf());
            goto error;
        }
    } else {
        if (!(p->client = ixp_nsmount(wmii_namespace))) {
            LS_FATALF(bd, "ixp_nsmount: %s: %s", wmii_namespace, ixp_errbuf());
            goto error;
        }
    }

    if (nwidgets > 1000) {
        LS_WARNF(bd, "# of widgets > 1000, expect wrong widget ordering");
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
        if (!(f = ixp_open(p->client, path, P9_OWRITE | P9_OCEXEC))) {
            LS_ERRF(bd, "ixp_open: %s: %s", path, ixp_errbuf());
            return false;
        }
    } else {
        if (!(f = ixp_create(p->client, path, 6 /* rw- */, P9_OWRITE | P9_OCEXEC))) {
            LS_ERRF(bd, "ixp_create: %s: %s", path, ixp_errbuf());
            return false;
        }
        p->exists[widget_idx] = 1;
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
    if (ixp_close(f) != 1) {
        // only produce a warning
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
        p->exists[widget_idx] = 0;
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

static
int
event_watcher(LuastatusBarlibData *bd, LuastatusBarlibEWFuncs funcs)
{
    Priv *p = bd->priv;

    if (!p->in) {
        return LUASTATUS_NONFATAL_ERR;
    }

    char *line = NULL;
    size_t line_alloc = 0;
    for (ssize_t nline; (nline = getline(&line, &line_alloc, p->in)) >= 0;) {
        char event[128];
        int button;
        size_t widget_idx;
        if (sscanf(line, "%127s %d LS%zu\n", event, &button, &widget_idx) != 3) {
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
        lua_State *L = funcs.call_begin(bd->userdata, widget_idx);
        // L: ?
        lua_newtable(L); // L: ? table
        lua_pushstring(L, evdetail); // L: ? table string
        lua_setfield(L, -2, "event"); // L: ? table
        lua_pushinteger(L, button); // L: ? table integer
        lua_setfield(L, -2, "button"); // L: ? table
        funcs.call_end(bd->userdata, widget_idx);
    }

    if (feof(p->in)) {
        LS_FATALF(bd, "(event watcher) event fd has been closed");
    } else {
        LS_WITH_ERRSTR(s, errno,
            LS_FATALF(bd, "(event watcher) cannot read from event fd: %s", s);
        );
    }
    free(line);
    return LUASTATUS_ERR;
}

LuastatusBarlibIface luastatus_barlib_iface_v1 = {
    .init = init,
    .set = set,
    .set_error = set_error,
    .event_watcher = event_watcher,
    .destroy = destroy,
};
