#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include "include/barlib.h"
#include "include/barlib_logf_macros.h"
#include "libls/string.h"
#include "libls/vector.h"
#include "libls/cstring_utils.h"
#include "libls/parse_int.h"
#include "libls/errno_utils.h"
#include "libls/io_utils.h"
#include "libls/lua_utils.h"
#include "libls/alloc_utils.h"

typedef struct {
    size_t nwidgets;
    LSString *bufs;
    char *sep;
    FILE *in;
    FILE *out;
    LSString luabuf;
} Priv;

void
priv_destroy(Priv *p)
{
    for (size_t i = 0; i < p->nwidgets; ++i) {
        LS_VECTOR_FREE(p->bufs[i]);
    }
    free(p->bufs);
    free(p->sep);
    if (p->in) {
        fclose(p->in);
    }
    if (p->out) {
        fclose(p->out);
    }
    LS_VECTOR_FREE(p->luabuf);
}

LuastatusBarlibInitResult
init(LuastatusBarlibData *bd, const char *const *opts, size_t nwidgets)
{
    Priv *p = bd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .nwidgets = nwidgets,
        .bufs = LS_XNEW(LSString, nwidgets),
        .sep = NULL,
        .in = NULL,
        .out = NULL,
        .luabuf = LS_VECTOR_NEW(),
    };

    // All the options may be passed multiple times!
    const char *sep = NULL;
    int in_fd = -1;
    int out_fd = -1;
    for (const char *const *s = opts; *s; ++s) {
        const char *v;
        if ((v = ls_strfollow(*s, "in_fd="))) {
            if ((in_fd = ls_full_parse_uint_cstr(v)) < 0) {
                LUASTATUS_FATALF(bd, "in_fd value is not a valid unsigned integer");
                goto error;
            }
        } else if ((v = ls_strfollow(*s, "out_fd="))) {
            if ((out_fd = ls_full_parse_uint_cstr(v)) < 0) {
                LUASTATUS_FATALF(bd, "out_fd value is not a valid unsigned integer");
                goto error;
            }
        } else if ((v = ls_strfollow(*s, "separator="))) {
            sep = v;
        } else {
            LUASTATUS_FATALF(bd, "unknown option '%s'", *s);
            goto error;
        }
    }
    p->sep = ls_xstrdup(sep ? sep : " | ");

    // check
    if (in_fd < 3) {
        LUASTATUS_FATALF(bd, "in_fd is not specified or less than 3");
        goto error;
    }
    if (out_fd < 3) {
        LUASTATUS_FATALF(bd, "out_fd is not specified or less than 3");
        goto error;
    }

    // open
    if (!(p->in = fdopen(in_fd, "r"))) {
        LS_WITH_ERRSTR(s, errno,
            LUASTATUS_FATALF(bd, "can't fdopen %d: %s", in_fd, s);
        );
        goto error;
    }
    if (!(p->out = fdopen(out_fd, "w"))) {
        LS_WITH_ERRSTR(s, errno,
            LUASTATUS_FATALF(bd, "can't fdopen %d: %s", out_fd, s);
        );
        goto error;
    }

    // make CLOEXEC
    if (ls_make_cloexec(in_fd) < 0) {
        LS_WITH_ERRSTR(s, errno,
            LUASTATUS_FATALF(bd, "can't make fd %d CLOEXEC: %s", in_fd, s);
        );
        goto error;
    }
    if (ls_make_cloexec(out_fd) < 0) {
        LS_WITH_ERRSTR(s, errno,
            LUASTATUS_FATALF(bd, "can't make fd %d CLOEXEC: %s", out_fd, s);
        );
        goto error;
    }

    return LUASTATUS_BARLIB_INIT_RESULT_OK;

error:
    priv_destroy(p);
    free(p);
    return LUASTATUS_BARLIB_INIT_RESULT_ERR;
}

int
l_escape(lua_State *L)
{
    // L: -
    size_t ns;
    // WARNING: luaL_check*() call luaL_error() on error, which, in turn, calls lua_error(),
    // which, it turn, does longjmp(). If this function should acquire any resource, it should be
    // done *after* calling luaL_check*().
    const char *s = luaL_checklstring(L, 1, &ns);

    LuastatusBarlibData *bd = lua_touserdata(L, lua_upvalueindex(1));
    Priv *p = bd->priv;

    LSString *buf = &p->luabuf;
    LS_VECTOR_CLEAR(*buf);

    // just replace all % with %%
    const char *t;
    while (ns && (t = memchr(s, '%', ns))) {
        const size_t nseg = t - s + 1;
        ls_string_append_b(buf, s, nseg);
        ls_string_append_c(buf, '%');
        ns -= nseg;
        s += nseg;
    }
    ls_string_append_b(buf, s, ns);

    lua_pushlstring(L, buf->data, buf->size); // L: string
    return 1;
}

void
register_funcs(LuastatusBarlibData *bd, lua_State *L)
{
    // L: table
    lua_pushlightuserdata(L, bd); // L: table bd
    lua_pushcclosure(L, l_escape, 1); // L: table bd l_escape
    ls_lua_rawsetf(L, "escape"); // L: table
}

bool
redraw(LuastatusBarlibData *bd)
{
    Priv *p = bd->priv;
    FILE *out = p->out;
    size_t n = p->nwidgets;
    LSString *bufs = p->bufs;
    char *sep = p->sep;

    bool first = true;
    for (size_t i = 0; i < n; ++i) {
        if (bufs[i].size) {
            if (!first) {
                fputs(sep, out);
            }
            fwrite(bufs[i].data, 1, bufs[i].size, out);
            first = false;
        }
    }
    fputc('\n', out);
    fflush(out);
    if (ferror(out)) {
        LS_WITH_ERRSTR(s, errno,
            LUASTATUS_FATALF(bd, "write error: %s", s);
        );
        return false;
    }
    return true;
}

LuastatusBarlibSetResult
set(LuastatusBarlibData *bd, lua_State *L, size_t widget_idx)
{
    Priv *p = bd->priv;
    int type = lua_type(L, -1);
    if (type == LUA_TSTRING) {
        size_t ns;
        const char *s = lua_tolstring(L, -1, &ns);
        ls_string_assign_b(&p->bufs[widget_idx], s, ns);
    } else if (type == LUA_TNIL) {
        LS_VECTOR_CLEAR(p->bufs[widget_idx]);
    } else {
        LUASTATUS_ERRF(bd, "expected string or nil, found %s", luaL_typename(L, -1));
        return LUASTATUS_BARLIB_SET_RESULT_NONFATAL_ERR;
    }
    if (!redraw(bd)) {
        return LUASTATUS_BARLIB_SET_RESULT_FATAL_ERR;
    }
    return LUASTATUS_BARLIB_SET_RESULT_OK;
}

LuastatusBarlibSetErrorResult
set_error(LuastatusBarlibData *bd, size_t widget_idx)
{
    Priv *p = bd->priv;
    ls_string_assign_s(&p->bufs[widget_idx], "%{B#f00}%{F#fff}(Error)%{B-}%{F-}");
    if (!redraw(bd)) {
        return LUASTATUS_BARLIB_SET_ERROR_RESULT_FATAL_ERR;
    }
    return LUASTATUS_BARLIB_SET_ERROR_RESULT_OK;
}

void
destroy(LuastatusBarlibData *bd)
{
    priv_destroy(bd->priv);
    free(bd->priv);
}

LuastatusBarlibIface luastatus_barlib_iface = {
    .init = init,
    .register_funcs = register_funcs,
    .set = set,
    .set_error = set_error,
    .destroy = destroy,
};
