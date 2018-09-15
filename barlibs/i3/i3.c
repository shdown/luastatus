#include <string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <lua.h>
#include <lauxlib.h>

#include "include/barlib_v1.h"
#include "include/sayf_macros.h"

#include "libls/string_.h"
#include "libls/vector.h"
#include "libls/alloc_utils.h"
#include "libls/parse_int.h"
#include "libls/cstring_utils.h"
#include "libls/errno_utils.h"
#include "libls/io_utils.h"
#include "libls/osdep.h"
#include "libls/lua_utils.h"

#include "priv.h"
#include "generator_utils.h"
#include "event_watcher.h"

static
void
destroy(LuastatusBarlibData *bd)
{
    Priv *p = bd->priv;
    for (size_t i = 0; i < p->nwidgets; ++i) {
        LS_VECTOR_FREE(p->bufs[i]);
    }
    free(p->bufs);
    ls_close(p->in_fd);
    if (p->out) {
        fclose(p->out);
    }
    LS_VECTOR_FREE(p->luabuf);
    free(p);
}

static
int
init(LuastatusBarlibData *bd, const char *const *opts, size_t nwidgets)
{
    Priv *p = bd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .nwidgets = nwidgets,
        .bufs = LS_XNEW(LSString, nwidgets),
        .in_fd = -1,
        .out = NULL,
        .noclickev = false,
        .noseps = false,
        .luabuf = LS_VECTOR_NEW(),
    };
    for (size_t i = 0; i < nwidgets; ++i) {
        LS_VECTOR_INIT_RESERVE(p->bufs[i], 1024);
    }

    // All the options may be passed multiple times!
    int in_fd = -1;
    int out_fd = -1;
    bool allow_stopping = false;
    for (const char *const *s = opts; *s; ++s) {
        const char *v;
        if ((v = ls_strfollow(*s, "in_fd="))) {
            if ((in_fd = ls_full_parse_uint_s(v)) < 0) {
                LS_FATALF(bd, "in_fd value is not a valid unsigned integer");
                goto error;
            }
        } else if ((v = ls_strfollow(*s, "out_fd="))) {
            if ((out_fd = ls_full_parse_uint_s(v)) < 0) {
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
        LS_WITH_ERRSTR(s, errno,
            LS_FATALF(bd, "can't fdopen %d: %s", out_fd, s);
        );
        goto error;
    }

    // make CLOEXEC
    if (ls_make_cloexec(in_fd) < 0) {
        LS_WITH_ERRSTR(s, errno,
            LS_FATALF(bd, "can't make fd %d CLOEXEC: %s", in_fd, s);
        );
        goto error;
    }
    if (ls_make_cloexec(out_fd) < 0) {
        LS_WITH_ERRSTR(s, errno,
            LS_FATALF(bd, "can't make fd %d CLOEXEC: %s", out_fd, s);
        );
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
        LS_WITH_ERRSTR(s, errno,
            LS_FATALF(bd, "write error: %s", s);
        );
        goto error;
    }

    return LUASTATUS_OK;

error:
    destroy(bd);
    return LUASTATUS_ERR;
}

static
bool
redraw(LuastatusBarlibData *bd)
{
    Priv *p = bd->priv;

    FILE *out = p->out;
    size_t n = p->nwidgets;
    LSString *bufs = p->bufs;

    putc_unlocked('[', out);
    bool first = true;
    for (size_t i = 0; i < n; ++i) {
        if (bufs[i].size) {
            if (!first) {
                putc_unlocked(',', out);
            }
            fwrite(bufs[i].data, 1, bufs[i].size, out);
            first = false;
        }
    }
    fputs("],\n", out);
    fflush(out);
    if (ferror(out)) {
        LS_WITH_ERRSTR(s, errno,
            LS_FATALF(bd, "write error: %s", s);
        );
        return false;
    }
    return true;
}

static
int
l_pango_escape(lua_State *L)
{
    size_t ns;
    // WARNING: luaL_check*() functions do a long jump on error!
    const char *s = luaL_checklstring(L, 1, &ns);

    luaL_Buffer b;
    luaL_buffinit(L, &b);

    size_t prev = 0;
    for (size_t i = 0; i < ns; ++i) {
        const char *esc;
        switch (s[i]) {
        case '&':  esc = "&amp;";   break;
        case '<':  esc = "&lt;";    break;
        case '>':  esc = "&gt;";    break;
        case '\'': esc = "&apos;";  break;
        case '"':  esc = "&quot;";  break;
        default: continue;
        }
        luaL_addlstring(&b, s + prev, i - prev);
        luaL_addstring(&b, esc);
        prev = i + 1;
    }
    luaL_addlstring(&b, s + prev, ns - prev);

    luaL_pushresult(&b);
    return 1;
}

static
void
register_funcs(LuastatusBarlibData *bd, lua_State *L)
{
    (void) bd;
    // L: table
    lua_pushcfunction(L, l_pango_escape); // L: table l_pango_escape
    ls_lua_rawsetf(L, "pango_escape"); // L: table
}

// Appends a JSON segment to
//     /((Priv *) bd->priv)->bufs[widget_idx]/
// from the table at position /table_pos/ on /L/'s stack.
static
bool
append_segment(LuastatusBarlibData *bd, lua_State *L, int table_pos, size_t widget_idx)
{
    Priv *p = bd->priv;
    LSString *s = &p->bufs[widget_idx];

    if (s->size) {
        ls_string_append_c(s, ',');
    }
    ls_string_append_f(s, "{\"name\":\"%zu\"", widget_idx);
    bool separator_key_found = false;
    LS_LUA_TRAVERSE(L, table_pos) {
        if (!lua_isstring(L, LS_LUA_KEY)) {
            LS_ERRF(bd, "segment key: expected string, found %s", luaL_typename(L, -2));
            return false;
        }
        const char *key = lua_tostring(L, LS_LUA_KEY);
        if (strcmp(key, "name") == 0) {
            LS_WARNF(bd, "segment: ignoring 'name', it is set automatically; use 'instance' "
                         "instead");
            continue;
        } else if (strcmp(key, "separator") == 0) {
            separator_key_found = true;
        }
        ls_string_append_c(s, ',');
        json_ls_string_append_escaped_s(s, key);
        ls_string_append_c(s, ':');
        switch (lua_type(L, LS_LUA_VALUE)) {
        case LUA_TNUMBER:
            if (!json_ls_string_append_number(s, lua_tonumber(L, LS_LUA_VALUE))) {
                LS_ERRF(bd, "segment entry '%s': invalid number (NaN/Inf)", key);
                return false;
            }
            break;
        case LUA_TSTRING:
            json_ls_string_append_escaped_s(s, lua_tostring(L, LS_LUA_VALUE));
            break;
        case LUA_TBOOLEAN:
            ls_string_append_s(s, lua_toboolean(L, LS_LUA_VALUE) ? "true" : "false");
            break;
        case LUA_TNIL:
            ls_string_append_s(s, "null");
            break;
        default:
            LS_ERRF(bd, "segment entry '%s': expected string, number, boolean or nil, found %s",
                key, luaL_typename(L, LS_LUA_VALUE));
            return false;
        }
    }
    if (p->noseps && !separator_key_found) {
        ls_string_append_s(s, ",\"separator\":false");
    }
    ls_string_append_c(s, '}');
    return true;
}

static
int
set(LuastatusBarlibData *bd, lua_State *L, size_t widget_idx)
{
    Priv *p = bd->priv;
    LS_VECTOR_CLEAR(p->bufs[widget_idx]);

    switch (lua_type(L, -1)) {
    case LUA_TNIL:
        break;
    case LUA_TTABLE:
        // L: table
        lua_pushnil(L); // L: table nil
        if (lua_next(L, -2)) {
            // table is not empty
            // L: table key value
            bool is_array = lua_isnumber(L, -2);
            lua_pop(L, 2); // L: table
            if (is_array) {
                LS_LUA_TRAVERSE(L, -1) {
                    switch (lua_type(L, LS_LUA_VALUE)) {
                    case LUA_TTABLE:
                        if (!append_segment(bd, L, LS_LUA_VALUE, widget_idx)) {
                            goto invalid_data;
                        }
                        break;
                    case LUA_TNIL:
                        break;
                    default:
                        LS_ERRF(bd, "array value: expected table or nil, found %s",
                            luaL_typename(L, LS_LUA_VALUE));
                        goto invalid_data;
                    }
                }
            } else {
                if (!append_segment(bd, L, -1, widget_idx)) {
                    goto invalid_data;
                }
            }
        } // else: L: table
        break;
    default:
        LS_ERRF(bd, "expected table or nil, found %s", luaL_typename(L, -1));
        goto invalid_data;
    }

    if (!redraw(bd)) {
        return LUASTATUS_ERR;
    }
    return LUASTATUS_OK;

invalid_data:
    // the buffer may contain an invalid JSON at this point; just clear it.
    LS_VECTOR_CLEAR(p->bufs[widget_idx]);
    return LUASTATUS_NONFATAL_ERR;
}

static
int
set_error(LuastatusBarlibData *bd, size_t widget_idx)
{
    Priv *p = bd->priv;
    LSString *s = &p->bufs[widget_idx];

    ls_string_assign_s(s, "{\"full_text\":\"(Error)\",\"color\":\"#ff0000\""
        ",\"background\":\"#000000\"");
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
