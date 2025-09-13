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

#include "event_watcher.h"

#include <stdlib.h>
#include <sys/types.h>
#include <stdbool.h>
#include <lua.h>
#include <yajl/yajl_parse.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "include/sayf_macros.h"

#include "libls/tls_ebuf.h"
#include "libls/parse_int.h"
#include "libls/strarr.h"
#include "libls/compdep.h"
#include "libls/alloc_utils.h"
#include "libls/freemem.h"

#include "priv.h"

// If this is to be incremented, /lua_checkstack()/ must be called at appropriate times, and the
// depth of the recursion in /push_object()/ be potentially limited somehow.
enum { DEPTH_LIMIT = 10 };

typedef struct {
    enum {
        TYPE_ARRAY_START,
        TYPE_ARRAY_END,
        TYPE_MAP_START,
        TYPE_MAP_END,

        TYPE_STRING_KEY,

        TYPE_STRING,
        TYPE_NUMBER,
        TYPE_BOOL,
        TYPE_NULL,
    } type;

    union {
        size_t str_idx;
        double num;
        bool flag;
    } as;
} Token;

typedef struct {
    Token *data;
    size_t size;
    size_t capacity;
} TokenList;

static inline TokenList token_list_new(void)
{
    return (TokenList) {NULL, 0, 0};
}

static inline void token_list_push(TokenList *x, Token token)
{
    if (x->size == x->capacity) {
        x->data = ls_x2realloc(x->data, &x->capacity, sizeof(Token));
    }
    x->data[x->size++] = token;
}

static inline void token_list_clear(TokenList *x)
{
    LS_FREEMEM(x->data, x->size, x->capacity);
    x->size = 0;
}

static inline void token_list_free(TokenList *x)
{
    free(x->data);
}

typedef struct {
    // Current JSON nesting depth. Before the initial '[', depth == -1.
    int depth;

    // Whether the last key (at depth == 1) was "name".
    bool last_key_is_name;

    // An array in which all the JSON strings, including keys, are stored.
    LS_StringArray strarr;

    // A flat list of current event's tokens.
    TokenList tokens;

    // Current event's widget index, or a negative value if is not known yet or invalid.
    int widget;

    LuastatusBarlibData *bd;
    LuastatusBarlibEWFuncs funcs;
} Context;

// Converts a JSON object that starts at the token with index /*index/ in /ctx->tokens/, to a Lua
// object, and pushes it onto /L/'s stack.
// Advances /*index/ so that it points to one token past the last token of the object.
static void push_object(lua_State *L, Context *ctx, size_t *index)
{
    Token t = ctx->tokens.data[*index];
    switch (t.type) {
    case TYPE_ARRAY_START:
        lua_newtable(L); // L: table
        ++*index;
        for (unsigned n = 1; ctx->tokens.data[*index].type != TYPE_ARRAY_END; ++n) {
            push_object(L, ctx, index); // L: table elem
            lua_rawseti(L, -2, n); // L: table
        }
        break;
    case TYPE_MAP_START:
        lua_newtable(L); // L: table
        ++*index;
        while (ctx->tokens.data[*index].type != TYPE_MAP_END) {
            Token key = ctx->tokens.data[*index];
            assert(key.type == TYPE_STRING_KEY);

            // To limit the maximum number of slots pushed onto /L/'s stack to /N + O(1)/, where /N/
            // is the maximum /ctx->depth/ encountered, we have to push the value first.
            // Unfortunately, /lua_settable()/ expects the key to be pushed first. So we simply swap
            // them with /lua_insert()/.

            ++*index;
            push_object(L, ctx, index); // L: table value

            size_t ns;
            const char *s = ls_strarr_at(ctx->strarr, key.as.str_idx, &ns);
            lua_pushlstring(L, s, ns); // L: table value key

            lua_insert(L, -2); // L: table key value
            lua_settable(L, -3); // L: table
        }
        break;
    case TYPE_STRING:
        {
            size_t ns;
            const char *s = ls_strarr_at(ctx->strarr, t.as.str_idx, &ns);
            lua_pushlstring(L, s, ns);
        }
        break;
    case TYPE_NUMBER:
        lua_pushnumber(L, t.as.num);
        break;
    case TYPE_BOOL:
        lua_pushboolean(L, t.as.flag);
        break;
    case TYPE_NULL:
        lua_pushnil(L);
        break;
    default:
        LS_UNREACHABLE();
    }
    // Now, /*index/ points to the last token of the object; increment it by one.
    ++*index;
}

static void flush(Context *ctx)
{
    Priv *p = ctx->bd->priv;

    if (ctx->widget >= 0 && (size_t) ctx->widget < p->nwidgets) {
        lua_State *L = ctx->funcs.call_begin(ctx->bd->userdata, ctx->widget);
        size_t index = 0;
        push_object(L, ctx, &index);
        assert(index == ctx->tokens.size);
        ctx->funcs.call_end(ctx->bd->userdata, ctx->widget);
    }

    // reset the context
    ctx->last_key_is_name = false;
    ls_strarr_clear(&ctx->strarr);
    token_list_clear(&ctx->tokens);
    ctx->widget = -1;
}

static int token_helper(Context *ctx, Token token)
{
    if (ctx->depth == -1) {
        if (token.type != TYPE_ARRAY_START) {
            LS_ERRF(ctx->bd, "(event watcher) expected '['");
            return 0;
        }
        ++ctx->depth;
    } else {
        if (ctx->depth == 0 && token.type != TYPE_MAP_START) {
            LS_ERRF(ctx->bd, "(event watcher) expected '{'");
            return 0;
        }
        token_list_push(&ctx->tokens, token);
        switch (token.type) {
        case TYPE_ARRAY_START:
        case TYPE_MAP_START:
            if (++ctx->depth >= DEPTH_LIMIT) {
                LS_ERRF(ctx->bd, "(event watcher) nesting depth limit exceeded");
                return 0;
            }
            break;

        case TYPE_ARRAY_END:
        case TYPE_MAP_END:
            if (--ctx->depth == 0) {
                flush(ctx);
            }
            break;

        default:
            break;
        }
    }
    return 1;
}

static inline size_t append_to_strarr(Context *ctx, const char *buf, size_t nbuf)
{
    ls_strarr_append(&ctx->strarr, buf, nbuf);
    return ls_strarr_size(ctx->strarr) - 1;
}

static int callback_null(void *vctx)
{
    return token_helper(vctx, (Token) {TYPE_NULL, {0}});
}

static int callback_boolean(void *vctx, int value)
{
    return token_helper(vctx, (Token) {TYPE_BOOL, {.flag = value}});
}

static int callback_integer(void *vctx, long long value)
{
    return token_helper(vctx, (Token) {TYPE_NUMBER, {.num = value}});
}

static int callback_double(void *vctx, double value)
{
    return token_helper(vctx, (Token) {TYPE_NUMBER, {.num = value}});
}

static int callback_string(void *vctx, const unsigned char *buf, size_t nbuf)
{
    Context *ctx = vctx;
    if (ctx->depth == 1 && ctx->last_key_is_name) {
        // parse error is OK here, /ctx->widget/ is checked in /flush()/.
        ctx->widget = ls_full_strtou_b((const char *) buf, nbuf);
    }

    return token_helper(ctx, (Token) {
        TYPE_STRING,
        {.str_idx = append_to_strarr(ctx, (const char *) buf, nbuf)}
    });
}

static int callback_start_map(void *vctx)
{
    return token_helper(vctx, (Token) {TYPE_MAP_START, {0}});
}

static int callback_map_key(void *vctx, const unsigned char *buf, size_t nbuf)
{
    Context *ctx = vctx;
    if (ctx->depth == 1) {
        ctx->last_key_is_name = (nbuf == 4 && memcmp(buf, "name", 4) == 0);
    }
    return token_helper(ctx, (Token) {
        TYPE_STRING_KEY,
        {.str_idx = append_to_strarr(ctx, (const char *) buf, nbuf)}
    });
}

static int callback_end_map(void *vctx)
{
    return token_helper(vctx, (Token) {TYPE_MAP_END, {0}});
}

static int callback_start_array(void *vctx)
{
    return token_helper(vctx, (Token) {TYPE_ARRAY_START, {0}});
}

static int callback_end_array(void *vctx)
{
    return token_helper(vctx, (Token) {TYPE_ARRAY_END, {0}});
}

int event_watcher(LuastatusBarlibData *bd, LuastatusBarlibEWFuncs funcs)
{
    Priv *p = bd->priv;
    if (p->noclickev)
        return LUASTATUS_NONFATAL_ERR;

    Context ctx = {
        .depth = -1,
        .last_key_is_name = false,
        .strarr = ls_strarr_new(),
        .tokens = token_list_new(),
        .widget = -1,
        .bd = bd,
        .funcs = funcs,
    };
    yajl_callbacks callbacks = {
        .yajl_null        = callback_null,
        .yajl_boolean     = callback_boolean,
        .yajl_integer     = callback_integer,
        .yajl_double      = callback_double,
        .yajl_string      = callback_string,
        .yajl_start_map   = callback_start_map,
        .yajl_map_key     = callback_map_key,
        .yajl_end_map     = callback_end_map,
        .yajl_start_array = callback_start_array,
        .yajl_end_array   = callback_end_array,
    };
    yajl_handle hand = yajl_alloc(&callbacks, NULL, &ctx);

    unsigned char buf[1024];
    while (1) {
        ssize_t nread = read(p->in_fd, buf, sizeof(buf));
        if (nread < 0) {
            LS_ERRF(bd, "(event watcher) read error: %s", ls_tls_strerror(errno));
            goto error;
        } else if (nread == 0) {
            LS_ERRF(bd, "(event watcher) i3bar closed its end of the pipe");
            goto error;
        }
        switch (yajl_parse(hand, buf, nread)) {
        case yajl_status_ok:
            break;
        case yajl_status_client_canceled:
            goto error;
        case yajl_status_error:
            {
                unsigned char *descr = yajl_get_error(hand, /*verbose*/ 1, buf, nread);
                LS_ERRF(bd, "(event watcher) yajl parse error: %s", (char *) descr);
                yajl_free_error(hand, descr);
            }
            goto error;
        }
    }

error:
    ls_strarr_destroy(ctx.strarr);
    token_list_free(&ctx.tokens);
    yajl_free(hand);
    return LUASTATUS_ERR;
}
