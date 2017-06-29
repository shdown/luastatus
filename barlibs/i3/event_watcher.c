#include "event_watcher.h"
#include <stdbool.h>
#include <lua.h>
#include <yajl/yajl_parse.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include "libls/errno_utils.h"
#include "libls/parse_int.h"
#include "libls/strarr.h"
#include "libls/vector.h"
#include "include/sayf_macros.h"
#include "priv.h"

typedef size_t StringIndex;

typedef struct {
    enum {
        TYPE_STRING,
        TYPE_DOUBLE,
        TYPE_BOOL,
    } type;
    union {
        StringIndex s_idx;
        double d;
        bool b;
    } u;
} Value;

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
    LSStringArray strs;
    LS_VECTOR_OF(KeyValue) params;
    StringIndex last_key_idx;
    int widget;

    LuastatusBarlibData *bd;
    LuastatusBarlibEWCallBegin *call_begin;
    LuastatusBarlibEWCallEnd *call_end;
} Context;

static inline
StringIndex
append_to_strs(Context *ctx, const char *buf, size_t nbuf)
{
    ls_strarr_append(&ctx->strs, buf, nbuf);
    return ls_strarr_size(ctx->strs) - 1;
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
    const char *key = ls_strarr_at(ctx->strs, ctx->last_key_idx, &nkey);
    if (nkey == 4 && memcmp("name", key, 4) == 0) {
        if (value.type != TYPE_STRING) {
            LS_ERRF(ctx->bd, "(event watcher) 'name' is not a string");
            return 0;
        }
        size_t nname;
        const char *name = ls_strarr_at(ctx->strs, value.u.s_idx, &nname);
        // parse error is OK here, ctx->widget is checked later
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
    Context *ctx = vctx;
    if (ctx->state != STATE_INSIDE_MAP) {
        LS_ERRF(ctx->bd, "(event watcher) unexpected null");
        return 0;
    }
    // nothing to do: Lua tables can't have nil values
    return 1;
}

static
int
callback_boolean(void *vctx, int value)
{
    return value_helper(vctx, (Value) {
        .type = TYPE_BOOL,
        .u = { .b = value },
    });
}

static
int
callback_integer(void *vctx, long long value)
{
    return value_helper(vctx, (Value) {
        .type = TYPE_DOUBLE,
        .u = { .d = value },
    });
}

static
int
callback_double(void *vctx, double value)
{
    return value_helper(vctx, (Value) {
        .type = TYPE_DOUBLE,
        .u = { .d = value },
    });
}

static
int
callback_string(void *vctx, const unsigned char *buf, size_t nbuf)
{
    return value_helper(vctx, (Value) {
        .type = TYPE_STRING,
        .u = { .s_idx = append_to_strs(vctx, (const char *) buf, nbuf) },
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
    ls_strarr_clear(&ctx->strs);
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
    ctx->last_key_idx = append_to_strs(vctx, (const char *) buf, nbuf);
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

#define PUSH_STRS_ELEM(Idx_) \
    do { \
        size_t ns_; \
        const char *s_ = ls_strarr_at(ctx->strs, Idx_, &ns_); \
        lua_pushlstring(L, s_, ns_); \
    } while (0)

    if (ctx->widget >= 0) {
        lua_State *L = ctx->call_begin(ctx->bd->userdata, ctx->widget);
        // L: -
        lua_newtable(L); // L: table
        for (size_t i = 0; i < ctx->params.size; ++i) {
            // L: table
            KeyValue kv = ctx->params.data[i];
            PUSH_STRS_ELEM(kv.key_idx); // L: table key
            switch (kv.value.type) {
            case TYPE_STRING:
                PUSH_STRS_ELEM(kv.value.u.s_idx); // L: table key value
                break;
            case TYPE_DOUBLE:
                lua_pushnumber(L, kv.value.u.d); // L: table key value
                break;
            case TYPE_BOOL:
                lua_pushboolean(L, kv.value.u.b); // L: table key value
            }
            lua_rawset(L, -3); // L: table
        }
        ctx->call_end(ctx->bd->userdata, ctx->widget);
#undef PUSH_STRS_ELEM
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
event_watcher(LuastatusBarlibData *bd,
              LuastatusBarlibEWCallBegin call_begin,
              LuastatusBarlibEWCallEnd call_end)
{
    Priv *p = bd->priv;
    if (p->noclickev) {
        return LUASTATUS_RES_NONFATAL_ERR;
    }

    Context ctx = {
        .state = STATE_EXPECTING_ARRAY_BEGIN,
        .strs = ls_strarr_new(),
        .params = LS_VECTOR_NEW(),
        .widget = -1,
        .bd = bd,
        .call_begin = call_begin,
        .call_end = call_end,
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
    ls_strarr_destroy(ctx.strs);
    LS_VECTOR_FREE(ctx.params);
    yajl_free(hand);
    return LUASTATUS_RES_ERR;
}
