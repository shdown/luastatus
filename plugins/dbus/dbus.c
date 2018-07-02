#include <lua.h>
#include <glib.h>
#include <gio/gio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <inttypes.h>
#include <assert.h>

#include "libls/vector.h"
#include "libls/compdep.h"
#include "libls/alloc_utils.h"

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"
#include "include/plugin_utils.h"

typedef struct {
    char *sender;
    char *interface;
    char *signal;
    char *object_path;
    char *arg0;
    GDBusSignalFlags flags;
} SignalSub;

typedef struct {
    LS_VECTOR_OF(SignalSub) signal_subs;
    int timeout_ms;
    bool greet;
} Priv;

// forward declaration
static
void
push_gvariant(lua_State *L, GVariant *var, unsigned recurlim);

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    for (size_t i = 0; i < p->signal_subs.size; ++i) {
        SignalSub s = p->signal_subs.data[i];
        free(s.sender);
        free(s.interface);
        free(s.signal);
        free(s.object_path);
        free(s.arg0);
    }
    LS_VECTOR_FREE(p->signal_subs);
    free(p);
}

static
int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .signal_subs = LS_VECTOR_NEW(),
        .timeout_ms = -1,
        .greet = false,
    };

    PU_MAYBE_VISIT_BOOL("greet", b,
        p->greet = b;
    );

    PU_MAYBE_VISIT_NUM("timeout", nsec,
        if (nsec >= 0) {
            double nmillis = nsec * 1000;
            if (nmillis > INT_MAX) {
                LS_FATALF(pd, "'timeout' is too large");
                goto error;
            }
            p->timeout_ms = nmillis;
        }
    );

    PU_TRAVERSE_TABLE("signals",
        SignalSub sub = {.flags = G_DBUS_SIGNAL_FLAGS_NONE};
        PU_TRAVERSE_TABLE_AT(LS_LUA_VALUE, "'signals' value",
            PU_MAYBE_VISIT_STR("sender",      s, sub.sender         = ls_xstrdup(s););
            PU_MAYBE_VISIT_STR("interface",   s, sub.interface      = ls_xstrdup(s););
            PU_MAYBE_VISIT_STR("signal",      s, sub.signal         = ls_xstrdup(s););
            PU_MAYBE_VISIT_STR("object_path", s, sub.object_path    = ls_xstrdup(s););
            PU_MAYBE_VISIT_STR("arg0",        s, sub.arg0           = ls_xstrdup(s););
            PU_MAYBE_VISIT_BOOL("arg0_match_namespace", b,
                if (b) {
                    sub.flags |= G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE;
                }
            );
            PU_MAYBE_VISIT_BOOL("arg0_match_path", b,
                if (b) {
                    sub.flags |= G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH;
                }
            );
        );
        LS_VECTOR_PUSH(p->signal_subs, sub);
    );

    return LUASTATUS_OK;

error:
    destroy(pd);
    return LUASTATUS_ERR;
}

typedef struct {
    LuastatusPluginData *pd;
    LuastatusPluginRunFuncs funcs;
} PluginRunArgs;

static
void
on_recur_lim(lua_State *L)
{
    lua_pushstring(L, "recursion limit reached");
}

static
void
push_gvariant_strlike(lua_State *L, GVariant *var)
{
    gsize ns;
    const gchar *s = g_variant_get_string(var, &ns);
    lua_pushlstring(L, s, ns);
}

static
void
push_gvariant_iterable(lua_State *L, GVariant *var, unsigned recurlim)
{
    if (!recurlim--) {
        on_recur_lim(L);
        return;
    }

    lua_newtable(L); // L: table
    GVariantIter iter;
    g_variant_iter_init(&iter, var);
    GVariant *elem;
    for (int n = 1; (elem = g_variant_iter_next_value(&iter)); ++n) {
        push_gvariant(L, elem, recurlim); // L: table value
        lua_rawseti(L, -2, n); // L: table
        g_variant_unref(elem);
    }
}

static
void
push_gvariant(lua_State *L, GVariant *var, unsigned recurlim)
{
    if (!recurlim--) {
        on_recur_lim(L);
        return;
    }

    if (!var) {
        lua_pushnil(L);
        return;
    }

    const char *kind;
    lua_newtable(L); // L: ? table

#define ALTSTR(GType_, LuaType_, CastTo_, Fmt_) \
    do { \
        lua_pushfstring(L, Fmt_, (CastTo_) g_variant_get_ ## GType_(var)); /* L: ? table str */ \
        lua_setfield(L, -2, "as_string"); /* L: ? table */ \
        lua_push ## LuaType_(L, g_variant_get_ ## GType_(var)); /* L: ? table value */ \
    } while (0)

    switch (g_variant_classify(var)) {
        case G_VARIANT_CLASS_BOOLEAN:
            kind = "boolean";
            lua_pushboolean(L, g_variant_get_boolean(var));
            break;
        case G_VARIANT_CLASS_BYTE:
            kind = "byte";
            ALTSTR(byte, integer, unsigned, "%u");
            break;
        case G_VARIANT_CLASS_INT16:
            kind = "int16";
            ALTSTR(int16, integer, int, "%d");
            break;
        case G_VARIANT_CLASS_UINT16:
            kind = "uint16";
            ALTSTR(uint16, integer, unsigned, "%u");
            break;
        case G_VARIANT_CLASS_INT32:
            kind = "int32";
            ALTSTR(int32, integer, int, "%d");
            break;
        case G_VARIANT_CLASS_UINT32:
            kind = "uint32";
            ALTSTR(uint32, integer, unsigned, "%u");
            break;
        case G_VARIANT_CLASS_INT64:
            kind = "int64";
            ALTSTR(int64, integer, int64_t, "%" PRId64);
            break;
        case G_VARIANT_CLASS_UINT64:
            kind = "uint64";
            ALTSTR(uint64, integer, uint64_t, "%" PRIu64);
            break;
        case G_VARIANT_CLASS_HANDLE:
            kind = "handle";
            lua_pushnil(L);
            break;
        case G_VARIANT_CLASS_DOUBLE:
            kind = "double";
            ALTSTR(double, number, double, "%g");
            break;
        case G_VARIANT_CLASS_STRING:
            kind = "string";
            push_gvariant_strlike(L, var);
            break;
        case G_VARIANT_CLASS_OBJECT_PATH:
            kind = "object_path";
            push_gvariant_strlike(L, var);
            break;
        case G_VARIANT_CLASS_SIGNATURE:
            kind = "signature";
            push_gvariant_strlike(L, var);
            break;
        case G_VARIANT_CLASS_VARIANT:
            kind = "variant";
            push_gvariant(L, g_variant_get_variant(var), recurlim);
            break;
        case G_VARIANT_CLASS_MAYBE:
            kind = "maybe";
            push_gvariant(L, g_variant_get_maybe(var), recurlim);
            break;
        case G_VARIANT_CLASS_ARRAY:
            kind = "array";
            push_gvariant_iterable(L, var, recurlim);
            break;
        case G_VARIANT_CLASS_TUPLE:
            kind = "tuple";
            push_gvariant_iterable(L, var, recurlim);
            break;
        case G_VARIANT_CLASS_DICT_ENTRY:
            kind = "dict_entry";
            push_gvariant_iterable(L, var, recurlim);
            break;
    }
    // L: ? table value
    lua_setfield(L, -2, "value"); // L: ? table
    lua_pushstring(L, kind); // L: ? table string
    lua_setfield(L, -2, "kind"); // L: ? table
#undef ALTSTR
}

static
void
callback_signal(
    LS_ATTR_UNUSED_ARG GDBusConnection *connection,
    const gchar *sender_name,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *signal_name,
    GVariant *parameters,
    gpointer user_data)
{
    PluginRunArgs args = *(PluginRunArgs *) user_data;
    lua_State *L = args.funcs.call_begin(args.pd->userdata);
    lua_newtable(L); // L: table
    lua_pushstring(L, "signal"); // L: table string
    lua_setfield(L, -2, "what"); // L: table
    lua_pushstring(L, sender_name); // L: table string
    lua_setfield(L, -2, "sender"); // L: table
    lua_pushstring(L, object_path); // L: table string
    lua_setfield(L, -2, "object_path"); // L: table
    lua_pushstring(L, interface_name); // L: table string
    lua_setfield(L, -2, "interface"); // L: table
    lua_pushstring(L, signal_name); // L: table string
    lua_setfield(L, -2, "signal"); // L: table
    push_gvariant(L, parameters, 5000); // L: table value
    lua_setfield(L, -2, "parameters"); // L: table
    args.funcs.call_end(args.pd->userdata);
}

static
gboolean
callback_timeout(gpointer user_data)
{
    PluginRunArgs args = *(PluginRunArgs *) user_data;
    lua_State *L = args.funcs.call_begin(args.pd->userdata);
    lua_newtable(L); // L: table
    lua_pushstring(L, "timeout"); // L: table string
    lua_setfield(L, -2, "what"); // L: table
    args.funcs.call_end(args.pd->userdata);
    return G_SOURCE_CONTINUE;
}

static
void
run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    GError *err = NULL;
    GDBusConnection *conn = NULL;
    GMainLoop *mainloop = NULL;
    GMainContext *context = g_main_context_get_thread_default();
    GSource *source = NULL;

    conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &err);
    if (err) {
        LS_FATALF(pd, "g_bus_get_sync: %s", err->message);
        goto error;
    }
    assert(conn);
    PluginRunArgs user_data = {.pd = pd, .funcs = funcs};
    for (size_t i = 0; i < p->signal_subs.size; ++i) {
        SignalSub s = p->signal_subs.data[i];
        g_dbus_connection_signal_subscribe(
            conn,
            s.sender,
            s.interface,
            s.signal,
            s.object_path,
            s.arg0,
            s.flags,
            callback_signal,
            &user_data,
            NULL);
    }
    if (p->timeout_ms >= 0) {
        source = g_timeout_source_new(p->timeout_ms);
        g_source_set_callback(source, callback_timeout, &user_data, NULL);
        if (g_source_attach(source, context) == 0) {
            LS_FATALF(pd, "g_source_attach failed\n");
            goto error;
        }
    }
    mainloop = g_main_loop_new(context, FALSE);

    if (p->greet) {
        lua_State *L = funcs.call_begin(pd->userdata);
        lua_newtable(L); // L: table
        lua_pushstring(L, "hello"); // L: table string
        lua_setfield(L, -2, "what"); // L: table
        funcs.call_end(pd->userdata);
    }
    g_main_loop_run(mainloop);

error:
    if (source) {
        g_source_unref(source);
    }
    if (context) {
        g_main_context_unref(context);
    }
    if (mainloop) {
        g_main_loop_unref(mainloop);
    }
    if (conn) {
        g_dbus_connection_close_sync(conn, NULL, NULL);
        g_object_unref(conn);
    }
    if (err) {
        g_error_free(err);
    }
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
