/*
 * Copyright (C) 2015-2020  luastatus developers
 *
 * This file is part of luastatus.
 *
 * luastatus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * luastatus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with luastatus.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stddef.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <errno.h>
#include <stdbool.h>

#include "include/barlib_v1.h"
#include "include/sayf_macros.h"

#include "libls/ls_string.h"
#include "libls/ls_cstring_utils.h"
#include "libls/ls_tls_ebuf.h"
#include "libls/ls_parse_int.h"
#include "libls/ls_io_utils.h"
#include "libls/ls_alloc_utils.h"

#include "markup_utils.h"

typedef struct {
    size_t nwidgets;

    LS_String *bufs;

    // Temporary buffer for secondary buffering, to avoid unneeded redraws.
    LS_String tmpbuf;

    char *sep;

    // /fdopen/'ed input file descriptor.
    FILE *in;

    // /fdopen/'ed output file descriptor.
    FILE *out;
} Priv;

static void destroy(LuastatusBarlibData *bd)
{
    Priv *p = bd->priv;
    for (size_t i = 0; i < p->nwidgets; ++i)
        ls_string_free(p->bufs[i]);
    free(p->bufs);
    ls_string_free(p->tmpbuf);
    free(p->sep);
    if (p->in)
        fclose(p->in);
    if (p->out)
        fclose(p->out);
    free(p);
}

static int init(LuastatusBarlibData *bd, const char *const *opts, size_t nwidgets)
{
    Priv *p = bd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .nwidgets = nwidgets,
        .bufs = LS_XNEW(LS_String, nwidgets),
        .tmpbuf = ls_string_new_reserve(512),
        .sep = NULL,
        .in = NULL,
        .out = NULL,
    };
    for (size_t i = 0; i < nwidgets; ++i)
        p->bufs[i] = ls_string_new_reserve(512);

    // All the options may be passed multiple times!
    const char *sep = NULL;
    int in_fd = -1;
    int out_fd = -1;
    for (const char *const *s = opts; *s; ++s) {
        const char *v;
        if ((v = ls_strfollow(*s, "in_fd="))) {
            if ((in_fd = ls_full_strtou(v)) < 0) {
                LS_FATALF(bd, "in_fd value is not a valid unsigned integer");
                goto error;
            }
        } else if ((v = ls_strfollow(*s, "out_fd="))) {
            if ((out_fd = ls_full_strtou(v)) < 0) {
                LS_FATALF(bd, "out_fd value is not a valid unsigned integer");
                goto error;
            }
        } else if ((v = ls_strfollow(*s, "separator="))) {
            sep = v;
        } else {
            LS_FATALF(bd, "unknown option '%s'", *s);
            goto error;
        }
    }
    p->sep = ls_xstrdup(sep ? sep : " | ");

    // we require /in_fd/ and /out_fd/ to be >=3 because making stdin/stdout/stderr CLOEXEC has very
    // bad consequences, and we just don't want to complicate the logic.
    if (in_fd < 3) {
        LS_FATALF(bd, "in_fd is not specified or less than 3");
        goto error;
    }
    if (out_fd < 3) {
        LS_FATALF(bd, "out_fd is not specified or less than 3");
        goto error;
    }

    // open
    if (!(p->in = fdopen(in_fd, "r"))) {
        LS_FATALF(bd, "can't fdopen %d: %s", in_fd, ls_tls_strerror(errno));
        goto error;
    }
    if (!(p->out = fdopen(out_fd, "w"))) {
        LS_FATALF(bd, "can't fdopen %d: %s", out_fd, ls_tls_strerror(errno));
        goto error;
    }

    // make CLOEXEC
    if (ls_make_cloexec(in_fd) < 0) {
        LS_FATALF(bd, "can't make fd %d CLOEXEC: %s", in_fd, ls_tls_strerror(errno));
        goto error;
    }
    if (ls_make_cloexec(out_fd) < 0) {
        LS_FATALF(bd, "can't make fd %d CLOEXEC: %s", out_fd, ls_tls_strerror(errno));
        goto error;
    }

    return LUASTATUS_OK;

error:
    destroy(bd);
    return LUASTATUS_ERR;
}

static int l_escape(lua_State *L)
{
    size_t ns;
    // WARNING: /luaL_check*()/ functions do a long jump on error!
    const char *s = luaL_checklstring(L, 1, &ns);

    push_escaped(L, s, ns);
    return 1;
}

static void register_funcs(LuastatusBarlibData *bd, lua_State *L)
{
    (void) bd;
    // L: table
    lua_pushcfunction(L, l_escape); // L: table l_escape
    lua_setfield(L, -2, "escape"); // L: table
}

static bool redraw(LuastatusBarlibData *bd)
{
    Priv *p = bd->priv;
    FILE *out = p->out;
    size_t n = p->nwidgets;
    LS_String *bufs = p->bufs;
    const char *sep = p->sep;

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
    putc_unlocked('\n', out);
    fflush(out);
    if (ferror(out)) {
        LS_FATALF(bd, "write error: %s", ls_tls_strerror(errno));
        return false;
    }
    return true;
}

static int set(LuastatusBarlibData *bd, lua_State *L, size_t widget_idx)
{
    Priv *p = bd->priv;
    LS_String *buf = &p->tmpbuf;
    ls_string_clear(buf);

    // L: ? data

    switch (lua_type(L, -1)) {
    case LUA_TNIL:
        break;

    case LUA_TSTRING:
        {
            size_t ns;
            const char *s = lua_tolstring(L, -1, &ns);
            append_sanitized_b(buf, widget_idx, s, ns);
        }
        break;

    case LUA_TTABLE:
        {
            const char *sep = p->sep;

            lua_pushnil(L); // L: ? data nil
            while (lua_next(L, -2)) {
                // L: ? data key value
                if (!lua_isstring(L, -1)) {
                    LS_ERRF(bd, "table value: expected string, found %s", luaL_typename(L, -1));
                    goto invalid_data;
                }
                size_t ns;
                const char *s = lua_tolstring(L, -1, &ns);
                if (buf->size && ns) {
                    ls_string_append_s(buf, sep);
                }
                append_sanitized_b(buf, widget_idx, s, ns);

                lua_pop(L, 1); // L: ? data key
            }
            // L: ? data
        }
        break;

    default:
        LS_ERRF(bd, "expected string, table or nil, found %s", luaL_typename(L, -1));
        goto invalid_data;
    }

    if (!ls_string_eq(*buf, p->bufs[widget_idx])) {
        ls_string_swap(buf, &p->bufs[widget_idx]);
        if (!redraw(bd)) {
            return LUASTATUS_ERR;
        }
    }
    return LUASTATUS_OK;

invalid_data:
    ls_string_clear(&p->bufs[widget_idx]);
    return LUASTATUS_NONFATAL_ERR;
}

static int set_error(LuastatusBarlibData *bd, size_t widget_idx)
{
    Priv *p = bd->priv;
    ls_string_assign_s(&p->bufs[widget_idx], "%{B#f00}%{F#fff}(Error)%{B-}%{F-}");
    if (!redraw(bd)) {
        return LUASTATUS_ERR;
    }
    return LUASTATUS_OK;
}

static int event_watcher(LuastatusBarlibData *bd, LuastatusBarlibEWFuncs funcs)
{
    Priv *p = bd->priv;

    char *buf = NULL;
    size_t nbuf = 256;

    for (ssize_t nread; (nread = getline(&buf, &nbuf, p->in)) >= 0;) {
        if (nread == 0 || buf[nread - 1] != '\n')
            continue;

        size_t ncommand;
        size_t widget_idx;
        const char *command = parse_command(buf, nread - 1, &ncommand, &widget_idx);
        if (!command)
            continue;
        if (widget_idx >= p->nwidgets) {
            continue;
        }
        lua_State *L = funcs.call_begin(bd->userdata, widget_idx);
        lua_pushlstring(L, command, ncommand);
        funcs.call_end(bd->userdata, widget_idx);
    }

    if (feof(p->in)) {
        LS_ERRF(bd, "lemonbar closed its pipe end");
    } else {
        LS_ERRF(bd, "read error: %s", ls_tls_strerror(errno));
    }

    free(buf);

    return LUASTATUS_ERR;
}

LuastatusBarlibIface luastatus_barlib_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .set = set,
    .set_error = set_error,
    .event_watcher = event_watcher,
    .destroy = destroy,
};
