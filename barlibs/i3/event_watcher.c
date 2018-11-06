#include "event_watcher.h"

#include <stdbool.h>
#include <lua.h>
#include <yajl/yajl_parse.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "include/sayf_macros.h"

#include "libls/cstring_utils.h"
#include "libls/parse_int.h"
#include "libls/strarr.h"
#include "libls/vector.h"
#include "libls/compdep.h"

#include "priv.h"

static const int DEPTH_LIMIT = 10;

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
    // Current JSON nesting depth. Before the initial '[', depth == -1.
    int depth;

    // Whether the last key (at depth == 1) was "name".
    bool last_key_is_name;

    // An array in which all the JSON strings, including keys, are stored.
    LSStringArray strarr;

    // Current event's parameters as key-value pairs.
    LS_VECTOR_OF(Token) tokens;

    // Current event's widget index, or a negative value if is not known yet or invalid.
    int widget;

    LuastatusBarlibData *bd;
    LuastatusBarlibEWFuncs funcs;
} Context;

static
void
push_object(lua_State *L, Context *ctx, size_t *index)
{
    Token t = ctx->tokens.data[*index];
    switch (t.type) {
    case TYPE_ARRAY_START:
        lua_newtable(L); // L: table
        ++*index;
        for (int n = 1; ctx->tokens.data[*index].type != TYPE_ARRAY_END; ++n) {
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
            size_t ns;
            const char *s = ls_strarr_at(ctx->strarr, key.as.str_idx, &ns);
            lua_pushlstring(L, s, ns); // L: table key

            ++*index;
            push_object(L, ctx, index); // L: table key value

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

static
void
flush(Context *ctx)
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
    LS_VECTOR_CLEAR(ctx->tokens);
    ctx->widget = -1;
}

static
int
token_helper(Context *ctx, Token token)
{
    switch (ctx->depth) {
    case -1:
        if (token.type != TYPE_ARRAY_START) {
            LS_ERRF(ctx->bd, "(event watcher) expected '['");
            return 0;
        }
        ++ctx->depth;
        break;

    case 0:
        if (token.type != TYPE_MAP_START) {
            LS_ERRF(ctx->bd, "(event watcher) expected '{'");
            return 0;
        }
        /* fall thru */
    default:
        LS_VECTOR_PUSH(ctx->tokens, token);
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
        break;
    }

    return 1;
}

static inline
size_t
append_to_strarr(Context *ctx, const char *buf, size_t nbuf)
{
    ls_strarr_append(&ctx->strarr, buf, nbuf);
    return ls_strarr_size(ctx->strarr) - 1;
}

static
int
callback_null(void *vctx)
{
    return token_helper(vctx, (Token) {TYPE_NULL, {0}});
}

static
int
callback_boolean(void *vctx, int value)
{
    return token_helper(vctx, (Token) {TYPE_BOOL, {.flag = value}});
}

static
int
callback_integer(void *vctx, long long value)
{
    return token_helper(vctx, (Token) {TYPE_NUMBER, {.num = value}});
}

static
int
callback_double(void *vctx, double value)
{
    return token_helper(vctx, (Token) {TYPE_NUMBER, {.num = value}});
}

static
int
callback_string(void *vctx, const unsigned char *buf, size_t nbuf)
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

static
int
callback_start_map(void *vctx)
{
    return token_helper(vctx, (Token) {TYPE_MAP_START, {0}});
}

static
int
callback_map_key(void *vctx, const unsigned char *buf, size_t nbuf)
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

static
int
callback_end_map(void *vctx)
{
    return token_helper(vctx, (Token) {TYPE_MAP_END, {0}});
}

static
int
callback_start_array(void *vctx)
{
    return token_helper(vctx, (Token) {TYPE_ARRAY_START, {0}});
}

static
int
callback_end_array(void *vctx)
{
    return token_helper(vctx, (Token) {TYPE_ARRAY_END, {0}});
}

int
event_watcher(LuastatusBarlibData *bd, LuastatusBarlibEWFuncs funcs)
{
    Priv *p = bd->priv;
    if (p->noclickev) {
        return LUASTATUS_NONFATAL_ERR;
    }

    Context ctx = {
        .depth = -1,
        .last_key_is_name = false,
        .strarr = ls_strarr_new(),
        .tokens = LS_VECTOR_NEW(),
        .widget = -1,
        .bd = bd,
        .funcs = funcs,
    };
    yajl_handle hand = yajl_alloc(
        &(yajl_callbacks) {
            .yajl_null        = callback_null,
            .yajl_boolean     = callback_boolean,
            .yajl_integer     = callback_integer,
            .yajl_double      = callback_double,
            .yajl_number      = NULL,
            .yajl_string      = callback_string,
            .yajl_start_map   = callback_start_map,
            .yajl_map_key     = callback_map_key,
            .yajl_end_map     = callback_end_map,
            .yajl_start_array = callback_start_array,
            .yajl_end_array   = callback_end_array,
        },
        NULL,
        &ctx);

    unsigned char buf[1024];
    while (1) {
        ssize_t nread = read(p->in_fd, buf, sizeof(buf));
        if (nread < 0) {
            LS_ERRF(bd, "(event watcher) read error: %s", ls_strerror_onstack(errno));
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
    LS_VECTOR_FREE(ctx.tokens);
    yajl_free(hand);
    return LUASTATUS_ERR;
}
