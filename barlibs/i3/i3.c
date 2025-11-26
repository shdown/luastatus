/*
 * Copyright (C) 2015-2025  luastatus developers
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

#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <lua.h>
#include <lauxlib.h>

#include "include/barlib_v1.h"
#include "include/sayf_macros.h"

#include "libls/ls_string.h"
#include "libls/ls_alloc_utils.h"
#include "libls/ls_parse_int.h"
#include "libls/ls_cstring_utils.h"
#include "libls/ls_tls_ebuf.h"
#include "libls/ls_io_utils.h"
#include "libsafe/safev.h"

#include "priv.h"
#include "event_watcher.h"
#include "escape_json_str.h"
#include "pango_escape.h"

static void destroy(LuastatusBarlibData *bd)
{
    Priv *p = bd->priv;
    for (size_t i = 0; i < p->nwidgets; ++i) {
        ls_string_free(p->bufs[i]);
    }
    free(p->bufs);
    ls_string_free(p->tmpbuf);
    ls_close(p->in_fd);
    if (p->out) {
        fclose(p->out);
    }
    free(p);
}

static int init(LuastatusBarlibData *bd, const char *const *opts, size_t nwidgets)
{
    Priv *p = bd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .nwidgets = nwidgets,
        .bufs = LS_XNEW(LS_String, nwidgets),
        .tmpbuf = ls_string_new_reserve(1024),
        .in_fd = -1,
        .out = NULL,
        .noclickev = false,
        .noseps = false,
    };
    for (size_t i = 0; i < nwidgets; ++i)
        p->bufs[i] = ls_string_new_reserve(1024);

    // All the options may be passed multiple times!
    int in_fd = -1;
    int out_fd = -1;
    bool allow_stopping = false;
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
        } else if (strcmp(*s, "no_click_events") == 0) {
            p->noclickev = true;
        } else if (strcmp(*s, "no_separators") == 0) {
            p->noseps = true;
        } else if (strcmp(*s, "allow_stopping") == 0) {
            allow_stopping = true;
        } else {
            LS_FATALF(bd, "unknown option '%s'", *s);
            goto error;
        }
    }

    // we require /in_fd/ and /out_fd/ to >=3 because making stdin/stdout/stderr CLOEXEC has very
    // bad consequences, and we just don't want to complicate the logic.
    if (in_fd < 3) {
        LS_FATALF(bd, "in_fd is not specified or less than 3");
        goto error;
    }
    if (out_fd < 3) {
        LS_FATALF(bd, "out_fd is not specified or less than 3");
        goto error;
    }

    // assign
    p->in_fd = in_fd;
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

    // print header
    fprintf(p->out, "{\"version\":1,\"click_events\":%s", p->noclickev ? "false" : "true");
    if (!allow_stopping) {
        fprintf(p->out, ",\"stop_signal\":0,\"cont_signal\":0");
    }
    fprintf(p->out, "}\n[\n");
    fflush(p->out);
    if (ferror(p->out)) {
        LS_FATALF(bd, "write error: %s", ls_tls_strerror(errno));
        goto error;
    }

    return LUASTATUS_OK;

error:
    destroy(bd);
    return LUASTATUS_ERR;
}

static bool redraw(LuastatusBarlibData *bd)
{
    Priv *p = bd->priv;

    FILE *out = p->out;
    size_t n = p->nwidgets;
    LS_String *bufs = p->bufs;

    putc('[', out);
    bool first = true;
    for (size_t i = 0; i < n; ++i) {
        if (bufs[i].size) {
            if (!first) {
                putc(',', out);
            }
            fwrite(bufs[i].data, 1, bufs[i].size, out);
            first = false;
        }
    }
    fputs("],\n", out);
    fflush(out);
    if (ferror(out)) {
        LS_FATALF(bd, "write error: %s", ls_tls_strerror(errno));
        return false;
    }
    return true;
}

static void append_to_lua_buf(void *ud, SAFEV segment)
{
    luaL_Buffer *b = ud;
    luaL_addlstring(b, SAFEV_ptr_UNSAFE(segment), SAFEV_len(segment));
}

static int l_pango_escape(lua_State *L)
{
    size_t ns;
    // WARNING: luaL_check*() functions do a long jump on error!
    const char *s = luaL_checklstring(L, 1, &ns);

    luaL_Buffer b;
    luaL_buffinit(L, &b);

    SAFEV v = SAFEV_new_UNSAFE(s, ns);
    pango_escape(v, append_to_lua_buf, &b);

    luaL_pushresult(&b);
    return 1;
}

static void register_funcs(LuastatusBarlibData *bd, lua_State *L)
{
    (void) bd;
    // L: table
    lua_pushcfunction(L, l_pango_escape); // L: table l_pango_escape
    lua_setfield(L, -2, "pango_escape"); // L: table
}

static inline bool append_json_number(LS_String *s, double value)
{
    if (!isfinite(value)) {
        return false;
    }
    ls_string_append_f(s, "%.20g", value);
    return true;
}

// Appends a JSON segment generated from table at the top of /L/'s stack, to
// /((Priv *) bd->priv)->tmpbuf/.
static bool append_segment(LuastatusBarlibData *bd, lua_State *L, size_t widget_idx)
{
    Priv *p = bd->priv;
    LS_String *dst = &p->tmpbuf;

    // add a "prologue"
    if (dst->size) {
        ls_string_append_c(dst, ',');
    }
    ls_string_append_f(dst, "{\"name\":\"%zu\"", widget_idx);

    bool has_separator_key = false;
    // L: ? table
    lua_pushnil(L); // L: ? table nil
    while (lua_next(L, -2)) {
        // L: ? table key value
        if (!lua_isstring(L, -2)) {
            LS_ERRF(bd, "segment key: expected string, found %s", luaL_typename(L, -2));
            return false;
        }
        const char *key = lua_tostring(L, -2);

        if (strcmp(key, "name") == 0) {
            LS_WARNF(bd, "segment: ignoring 'name', it is set automatically; use 'instance' "
                         "instead");
            goto next_entry;
        }

        if (strcmp(key, "separator") == 0) {
            has_separator_key = true;
        }

        ls_string_append_c(dst, ',');
        append_json_escaped_str(dst, SAFEV_new_from_cstr_UNSAFE(key));
        ls_string_append_c(dst, ':');

        switch (lua_type(L, -1)) {
        case LUA_TNUMBER:
            {
                double val = lua_tonumber(L, -1);
                if (!append_json_number(dst, val)) {
                    LS_ERRF(bd, "segment entry '%s': invalid number (NaN/Inf)", key);
                    return false;
                }
            }
            break;
        case LUA_TSTRING:
            {
                size_t ns;
                const char *s = lua_tolstring(L, -1, &ns);
                append_json_escaped_str(dst, SAFEV_new_UNSAFE(s, ns));
            }
            break;
        case LUA_TBOOLEAN:
            {
                bool val = lua_toboolean(L, -1);
                ls_string_append_s(dst, val ? "true" : "false");
            }
            break;
        case LUA_TNIL:
            ls_string_append_s(dst, "null");
            break;
        default:
            LS_ERRF(bd, "segment entry '%s': expected string, number, boolean or nil, found %s",
                    key, luaL_typename(L, -1));
            return false;
        }

next_entry:
        lua_pop(L, 1); // L: ? table key
    }
    // L: ? table

    // add an "epilogue"
    if (p->noseps && !has_separator_key) {
        ls_string_append_s(dst, ",\"separator\":false");
    }
    ls_string_append_c(dst, '}');

    return true;
}

static int set(LuastatusBarlibData *bd, lua_State *L, size_t widget_idx)
{
    Priv *p = bd->priv;
    ls_string_clear(&p->tmpbuf);

    // L: ? data

    switch (lua_type(L, -1)) {
    case LUA_TNIL:
        break;

    case LUA_TTABLE:
        lua_pushnil(L); // L: ? data nil
        if (lua_next(L, -2)) {
            // The table is not empty.
            // L: ? data key value
            bool is_array = lua_isnumber(L, -2);
            if (is_array) {
                do {
                    // L: ? data key value
                    switch (lua_type(L, -1)) {
                    case LUA_TTABLE:
                        if (!append_segment(bd, L, widget_idx)) {
                            goto invalid_data;
                        }
                        break;
                    case LUA_TNIL:
                        break;
                    default:
                        LS_ERRF(bd, "array value: expected table or nil, found %s",
                                luaL_typename(L, -1));
                        goto invalid_data;
                    }
                    lua_pop(L, 1); // L: ? data key
                } while (lua_next(L, -2));
                // L: ? data
            } else {
                lua_pop(L, 2); // L: ? data
                if (!append_segment(bd, L, widget_idx)) {
                    goto invalid_data;
                }
            }
            // L: ? data
        }
        // L: ? data
        break;

    default:
        LS_ERRF(bd, "expected table or nil, found %s", luaL_typename(L, -1));
        goto invalid_data;
    }

    if (!ls_string_eq(p->tmpbuf, p->bufs[widget_idx])) {
        ls_string_swap(&p->tmpbuf, &p->bufs[widget_idx]);
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
    LS_String *s = &p->bufs[widget_idx];

    ls_string_assign_s(
        s, "{\"full_text\":\"(Error)\",\"color\":\"#ff0000\",\"background\":\"#000000\"");
    if (p->noseps) {
        ls_string_append_s(s, ",\"separator\":false");
    }
    ls_string_append_c(s, '}');

    if (!redraw(bd)) {
        return LUASTATUS_ERR;
    }
    return LUASTATUS_OK;
}

LuastatusBarlibIface luastatus_barlib_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .set = set,
    .set_error = set_error,
    .event_watcher = event_watcher,
    .destroy = destroy,
};
