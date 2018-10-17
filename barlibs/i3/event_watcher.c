#include "event_watcher.h"

#include <stdbool.h>
#include <lua.h>
#include <yajl/yajl_parse.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "include/sayf_macros.h"

#include "libls/errno_utils.h"
#include "libls/parse_int.h"
#include "libls/strarr.h"
#include "libls/vector.h"

#include "priv.h"

// We store all string keys and values in the /strarr/ field of /Context/ structure; this type is
// used for an index of the string in that /LSStringArray/.
typedef size_t StringIndex;

// Type for a JSON value awaiting to be converted to a Lua value.
typedef struct {
    enum {
        TYPE_STRING,
        TYPE_NUMBER,
        TYPE_BOOL,
        TYPE_NULL,
    } type;
    union {
        StringIndex str_idx;
        double num;
        bool flag;
    } as;
} Value;

// Type for a JSON key (JSON only supports string keys).
typedef struct {
    StringIndex key_idx;
    Value value;
} KeyValue;

typedef struct {
    enum {
        STATE_EXPECTING_ARRAY_BEGIN,
        STATE_EXPECTING_MAP_BEGIN,
        STATE_INSIDE_MAP,
    } state;
    // An array in which the JSON strings, including keys, are stored.
    LSStringArray strarr;
    // Current event's parameters as key-value pairs.
    LS_VECTOR_OF(KeyValue) params;
    // Index in /strarr/ of the last encountered JSON key.
    StringIndex last_key_idx;
    // Current event's widget index, or a negative value if is not known yet or invalid.
    int widget;

    LuastatusBarlibData *bd;
    LuastatusBarlibEWFuncs funcs;
} Context;

static inline
StringIndex
append_to_strarr(Context *ctx, const char *buf, size_t nbuf)
{
    ls_strarr_append(&ctx->strarr, buf, nbuf);
    return ls_strarr_size(ctx->strarr) - 1;
}

static
int
value_helper(Context *ctx, Value value)
{
    if (ctx->state != STATE_INSIDE_MAP) {
        LS_ERRF(ctx->bd, "(event watcher) unexpected JSON value");
        return 0;
    }
    size_t nkey;
    const char *key = ls_strarr_at(ctx->strarr, ctx->last_key_idx, &nkey);
    if (nkey == 4 && memcmp("name", key, 4) == 0) {
        if (value.type != TYPE_STRING) {
            LS_ERRF(ctx->bd, "(event watcher) 'name' is not a string");
            return 0;
        }
        size_t nname;
        const char *name = ls_strarr_at(ctx->strarr, value.as.str_idx, &nname);
        // parse error is OK here, /ctx->widget/ is checked later
        ctx->widget = ls_full_parse_uint_b(name, nname);
    } else {
        LS_VECTOR_PUSH(ctx->params, ((KeyValue) {
            .key_idx = ctx->last_key_idx,
            .value = value,
        }));
    }
    return 1;
}

static
int
callback_null(void *vctx)
{
    return value_helper(vctx, (Value) {
        .type = TYPE_NULL,
    });
}

static
int
callback_boolean(void *vctx, int value)
{
    return value_helper(vctx, (Value) {
        .type = TYPE_BOOL,
        .as = { .flag = value },
    });
}

static
int
callback_integer(void *vctx, long long value)
{
    return value_helper(vctx, (Value) {
        .type = TYPE_NUMBER,
        .as = { .num = value },
    });
}

static
int
callback_double(void *vctx, double value)
{
    return value_helper(vctx, (Value) {
        .type = TYPE_NUMBER,
        .as = { .num = value },
    });
}

static
int
callback_string(void *vctx, const unsigned char *buf, size_t nbuf)
{
    return value_helper(vctx, (Value) {
        .type = TYPE_STRING,
        .as = { .str_idx = append_to_strarr(vctx, (const char *) buf, nbuf) },
    });
}

static
int
callback_start_map(void *vctx)
{
    Context *ctx = vctx;
    if (ctx->state != STATE_EXPECTING_MAP_BEGIN) {
        LS_ERRF(ctx->bd, "(event watcher) unexpected {");
        return 0;
    }
    ctx->state = STATE_INSIDE_MAP;
    ls_strarr_clear(&ctx->strarr);
    LS_VECTOR_CLEAR(ctx->params);
    ctx->widget = -1;
    return 1;
}

static
int
callback_map_key(void *vctx, const unsigned char *buf, size_t nbuf)
{
    Context *ctx = vctx;
    if (ctx->state != STATE_INSIDE_MAP) {
        LS_ERRF(ctx->bd, "(event watcher) unexpected map key");
        return 0;
    }
    ctx->last_key_idx = append_to_strarr(vctx, (const char *) buf, nbuf);
    return 1;
}

static
int
callback_end_map(void *vctx)
{
    Context *ctx = vctx;
    if (ctx->state != STATE_INSIDE_MAP) {
        LS_ERRF(ctx->bd, "(event watcher) unexpected }");
        return 0;
    }
    ctx->state = STATE_EXPECTING_MAP_BEGIN;

    Priv *p = ctx->bd->priv;
    if (ctx->widget >= 0 && (size_t) ctx->widget < p->nwidgets) {
        lua_State *L = ctx->funcs.call_begin(ctx->bd->userdata, ctx->widget);
        // L: -
        lua_createtable(L, 0, ctx->params.size); // L: table
        for (size_t i = 0; i < ctx->params.size; ++i) {
            // L: table
            KeyValue kv = ctx->params.data[i];

            size_t nkey;
            const char *key = ls_strarr_at(ctx->strarr, kv.key_idx, &nkey);
            lua_pushlstring(L, key, nkey); // L: table key

            switch (kv.value.type) {
            case TYPE_STRING:
                {
                    size_t nstr;
                    const char *str = ls_strarr_at(ctx->strarr, kv.value.as.str_idx, &nstr);
                    lua_pushlstring(L, str, nstr); // L: table key value
                }
                break;
            case TYPE_NUMBER:
                lua_pushnumber(L, kv.value.as.num); // L: table key value
                break;
            case TYPE_BOOL:
                lua_pushboolean(L, kv.value.as.flag); // L: table key value
                break;
            case TYPE_NULL:
                lua_pushnil(L); // L: table key value
                break;
            }
            lua_rawset(L, -3); // L: table
        }
        ctx->funcs.call_end(ctx->bd->userdata, ctx->widget);
    }
    return 1;
}

static
int
callback_start_array(void *vctx)
{
    Context *ctx = vctx;
    if (ctx->state != STATE_EXPECTING_ARRAY_BEGIN) {
        LS_ERRF(ctx->bd, "(event watcher) unexpected [");
        return 0;
    }
    ctx->state = STATE_EXPECTING_MAP_BEGIN;
    return 1;
}

static
int
callback_end_array(void *vctx)
{
    Context *ctx = vctx;
    LS_ERRF(ctx->bd, "(event watcher) unexpected ]");
    return 0;
}

int
event_watcher(LuastatusBarlibData *bd, LuastatusBarlibEWFuncs funcs)
{
    Priv *p = bd->priv;
    if (p->noclickev) {
        return LUASTATUS_NONFATAL_ERR;
    }

    Context ctx = {
        .state = STATE_EXPECTING_ARRAY_BEGIN,
        .strarr = ls_strarr_new(),
        .params = LS_VECTOR_NEW(),
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
            LS_WITH_ERRSTR(s, errno,
                LS_ERRF(bd, "(event watcher) read error: %s", s);
            );
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
    LS_VECTOR_FREE(ctx.params);
    yajl_free(hand);
    return LUASTATUS_ERR;
}
