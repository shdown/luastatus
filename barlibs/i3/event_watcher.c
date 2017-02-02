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
#include "libls/string.h"
#include "libls/vector.h"
#include "include/barlib_logf_macros.h"
#include "priv.h"

typedef struct {
    size_t offset, size;
} EventWatcherString;

typedef struct {
    enum {
        EVENT_WATCHER_VALUE_TYPE_STRING,
        EVENT_WATCHER_VALUE_TYPE_DOUBLE,
        EVENT_WATCHER_VALUE_TYPE_BOOL,
    } type;
    union {
        EventWatcherString s;
        double d;
        bool b;
    } u;
} EventWatcherValue;

typedef struct {
    EventWatcherString key;
    EventWatcherValue value;
} EventWatcherKeyValue;

typedef struct {
    LuastatusBarlibData *bd;
    LuastatusBarlibEWCallBegin *call_begin;
    LuastatusBarlibEWCallEnd *call_end;

    enum {
        EVENT_WATCHER_STATE_EXPECTING_ARRAY_BEGIN,
        EVENT_WATCHER_STATE_EXPECTING_MAP_BEGIN,
        EVENT_WATCHER_STATE_INSIDE_MAP,
    } state;

    LSString buf;
    EventWatcherString last_key;
    LS_VECTOR_OF(EventWatcherKeyValue) params;
    int widget_idx;
} EventWatcherCtx;

static inline
EventWatcherString
append_to_buf(EventWatcherCtx *ctx, const char *buf, size_t nbuf)
{
    size_t offset = ctx->buf.size;
    ls_string_append_b(&ctx->buf, buf, nbuf);
    return (EventWatcherString) {.offset = offset, .size = nbuf};
}

static
int
callback_value_helper(EventWatcherCtx *ctx, EventWatcherValue value)
{
    if (ctx->state != EVENT_WATCHER_STATE_INSIDE_MAP) {
        LUASTATUS_ERRF(ctx->bd, "(event watcher) unexpected JSON value");
        return 0;
    }
    if (ctx->last_key.size == 4 &&
        memcmp("name", ctx->buf.data + ctx->last_key.offset, 4) == 0)
    {
        if (value.type != EVENT_WATCHER_VALUE_TYPE_STRING) {
            LUASTATUS_ERRF(ctx->bd, "(event watcher) 'name' is not a string");
            return 0;
        }
        // parse error is OK here, widget_idx is checked later
        ctx->widget_idx = ls_full_parse_uint(ctx->buf.data + value.u.s.offset, value.u.s.size);
    } else {
        LS_VECTOR_PUSH(ctx->params, ((EventWatcherKeyValue) {
            .key = ctx->last_key,
            .value = value,
        }));
    }
    return 1;
}

static
int
callback_null(void *vctx)
{
    EventWatcherCtx *ctx = vctx;
    if (ctx->state != EVENT_WATCHER_STATE_INSIDE_MAP) {
        LUASTATUS_ERRF(ctx->bd, "(event watcher) unexpected null");
        return 0;
    }
    // nothing to do: Lua tables can't have nil values
    return 1;
}

static
int
callback_boolean(void *vctx, int value)
{
    return callback_value_helper(vctx, (EventWatcherValue) {
        .type = EVENT_WATCHER_VALUE_TYPE_BOOL,
        .u = { .b = value },
    });
}

static
int
callback_integer(void *vctx, long long value)
{
    return callback_value_helper(vctx, (EventWatcherValue) {
        .type = EVENT_WATCHER_VALUE_TYPE_DOUBLE,
        .u = { .d = value },
    });
}

static
int
callback_double(void *vctx, double value)
{
    return callback_value_helper(vctx, (EventWatcherValue) {
        .type = EVENT_WATCHER_VALUE_TYPE_DOUBLE,
        .u = { .d = value },
    });
}

static
int
callback_string(void *vctx, const unsigned char *buf, size_t nbuf)
{
    return callback_value_helper(vctx, (EventWatcherValue) {
        .type = EVENT_WATCHER_VALUE_TYPE_STRING,
        .u = { .s = append_to_buf(vctx, (const char*) buf, nbuf) },
    });
}

static
int
callback_start_map(void *vctx)
{
    EventWatcherCtx *ctx = vctx;
    if (ctx->state != EVENT_WATCHER_STATE_EXPECTING_MAP_BEGIN) {
        LUASTATUS_ERRF(ctx->bd, "(event watcher) unexpected {");
        return 0;
    }
    ctx->state = EVENT_WATCHER_STATE_INSIDE_MAP;
    LS_VECTOR_CLEAR(ctx->buf);
    LS_VECTOR_CLEAR(ctx->params);
    ctx->widget_idx = -1;
    return 1;
}

static
int
callback_map_key(void *vctx, const unsigned char *buf, size_t nbuf)
{
    EventWatcherCtx *ctx = vctx;
    if (ctx->state != EVENT_WATCHER_STATE_INSIDE_MAP) {
        LUASTATUS_ERRF(ctx->bd, "(event watcher) unexpected map key");
        return 0;
    }
    ctx->last_key = append_to_buf(vctx, (const char*) buf, nbuf);
    return 1;
}

static
int
callback_end_map(void *vctx)
{
    EventWatcherCtx *ctx = vctx;
    if (ctx->state != EVENT_WATCHER_STATE_INSIDE_MAP) {
        LUASTATUS_ERRF(ctx->bd, "(event watcher) unexpected }");
        return 0;
    }
    ctx->state = EVENT_WATCHER_STATE_EXPECTING_MAP_BEGIN;

    if (ctx->widget_idx >= 0) {
        lua_State *L = ctx->call_begin(ctx->bd->userdata, ctx->widget_idx);
        // L: -
        lua_newtable(L); // L: table
        for (size_t i = 0; i < ctx->params.size; ++i) {
            // L: table
            EventWatcherKeyValue kv = ctx->params.data[i];
#define PUSH_EW_STR(S_) \
    lua_pushlstring(L, ctx->buf.data + (S_).offset, (S_).size)
            PUSH_EW_STR(kv.key); // L: table key
            switch (kv.value.type) {
            case EVENT_WATCHER_VALUE_TYPE_STRING:
                PUSH_EW_STR(kv.value.u.s); // L: table key value
                break;
            case EVENT_WATCHER_VALUE_TYPE_DOUBLE:
                lua_pushnumber(L, kv.value.u.d); // L: table key value
                break;
            case EVENT_WATCHER_VALUE_TYPE_BOOL:
                lua_pushboolean(L, kv.value.u.b); // L: table key value
            }
            lua_rawset(L, -3); // L: table
        }
        ctx->call_end(ctx->bd->userdata, ctx->widget_idx);
#undef PUSH_EW_STR
    }
    return 1;
}

static
int
callback_start_array(void *vctx)
{
    EventWatcherCtx *ctx = vctx;
    if (ctx->state != EVENT_WATCHER_STATE_EXPECTING_ARRAY_BEGIN) {
        LUASTATUS_ERRF(ctx->bd, "(event watcher) unexpected [");
        return 0;
    }
    ctx->state = EVENT_WATCHER_STATE_EXPECTING_MAP_BEGIN;
    return 1;
}

static
int
callback_end_array(void *vctx)
{
    EventWatcherCtx *ctx = vctx;
    LUASTATUS_ERRF(ctx->bd, "(event watcher) unexpected ]");
    return 0;
}

LuastatusBarlibEWResult
event_watcher(LuastatusBarlibData *bd,
              LuastatusBarlibEWCallBegin call_begin,
              LuastatusBarlibEWCallEnd call_end)
{
    Priv *p = bd->priv;
    if (p->noclickev) {
        return LUASTATUS_BARLIB_EW_RESULT_NO_MORE_EVENTS;
    }

    EventWatcherCtx ctx = {
        .bd = bd,
        .call_begin = call_begin,
        .call_end = call_end,
        .state = EVENT_WATCHER_STATE_EXPECTING_ARRAY_BEGIN,
        .buf = LS_VECTOR_NEW(),
        .params = LS_VECTOR_NEW(),
        .widget_idx = -1,
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
                LUASTATUS_ERRF(bd, "(event watcher) read error: %s", s);
            );
            goto error;
        } else if (nread == 0) {
            LUASTATUS_ERRF(bd, "(event watcher) i3bar closed its end of the pipe");
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
                LUASTATUS_ERRF(bd, "(event watcher) yajl parse error: %s", (char*) descr);
                yajl_free_error(hand, descr);
            }
            goto error;
        }
    }

error:
    LS_VECTOR_FREE(ctx.buf);
    LS_VECTOR_FREE(ctx.params);
    yajl_free(hand);
    return LUASTATUS_BARLIB_EW_RESULT_FATAL_ERR;
}
