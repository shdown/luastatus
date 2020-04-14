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

#include <lua.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include "libls/vector.h"
#include "libls/compdep.h"
#include "libls/alloc_utils.h"
#include "libls/lua_utils.h"

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"
#include "include/plugin_utils.h"

#include "marshal.h"

typedef struct {
    char *sender;
    char *interface;
    char *signal;
    char *object_path;
    char *arg0;
    GDBusSignalFlags flags;
} SignalSub;

#define SIGNAL_SUB_NEW() {.flags = G_DBUS_SIGNAL_FLAGS_NONE}

static
void
signal_sub_free(SignalSub s)
{
    free(s.sender);
    free(s.interface);
    free(s.signal);
    free(s.object_path);
    free(s.arg0);
}

typedef LS_VECTOR_OF(SignalSub) SubList;

static
void
sub_list_free(SubList sl)
{
    for (size_t i = 0; i < sl.size; ++i) {
        signal_sub_free(sl.data[i]);
    }
    LS_VECTOR_FREE(sl);
}

typedef struct {
    SubList session_subs;
    SubList system_subs;
    int timeout_ms;
    bool greet;
} Priv;

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    sub_list_free(p->session_subs);
    sub_list_free(p->system_subs);
    free(p);
}

static
int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .session_subs = LS_VECTOR_NEW(),
        .system_subs = LS_VECTOR_NEW(),
        .timeout_ms = -1,
        .greet = false,
    };
    SignalSub sub = SIGNAL_SUB_NEW();

    PU_MAYBE_VISIT_BOOL_FIELD(-1, "greet", "'greet'", b,
        p->greet = b;
    );

    PU_MAYBE_VISIT_NUM_FIELD(-1, "timeout", "'timeout'", nsec,
        // Note: this also implicitly checks that /nsec/ is not NaN.
        if (nsec >= 0) {
            double nmillis = nsec * 1000;
            if (nmillis > INT_MAX) {
                LS_FATALF(pd, "'timeout' is too large");
                goto error;
            }
            p->timeout_ms = nmillis;
        }
    );

    PU_VISIT_TABLE_FIELD(-1, "signals", "'signals'",
        SubList *dest = &p->session_subs;

        PU_MAYBE_VISIT_STR_FIELD(LS_LUA_VALUE, "sender", "'signals' element, 'sender'", s,
            sub.sender = ls_xstrdup(s);
        );

        PU_MAYBE_VISIT_STR_FIELD(LS_LUA_VALUE, "interface", "'signals' element, 'interface'", s,
            sub.interface = ls_xstrdup(s);
        );

        PU_MAYBE_VISIT_STR_FIELD(LS_LUA_VALUE, "signal", "'signals' element, 'signal'", s,
            sub.signal = ls_xstrdup(s);
        );

        PU_MAYBE_VISIT_STR_FIELD(LS_LUA_VALUE, "object_path", "'signals' element, 'object_path'", s,
            sub.object_path = ls_xstrdup(s);
        );

        PU_MAYBE_VISIT_STR_FIELD(LS_LUA_VALUE, "arg0", "'signals' element', 'arg0'", s,
            sub.arg0 = ls_xstrdup(s);
        );

        PU_MAYBE_VISIT_TABLE_FIELD(LS_LUA_VALUE, "flags", "'signals' element, 'flags'",

            PU_VISIT_STR(LS_LUA_VALUE, "'signals' element, 'flags' subelement", s,
                if (strcmp(s, "match_arg0_namespace") == 0) {
                    sub.flags |= G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE;
                } else if (strcmp(s, "match_arg0_path") == 0) {
                    sub.flags |= G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH;
                } else {
                    LS_FATALF(pd, "'signals' element, 'flags': unknown flag: '%s'", s);
                    goto error;
                }
            );
        );

        PU_MAYBE_VISIT_STR_FIELD(LS_LUA_VALUE, "bus", "'signals' element, 'bus'", s,
            if (strcmp(s, "session") == 0) {
                dest = &p->session_subs;
            } else if (strcmp(s, "system") == 0) {
                dest = &p->system_subs;
            } else {
                LS_FATALF(pd, "'signals' element, 'bus': unknown bus: '%s'", s);
                goto error;
            }
        );

        LS_VECTOR_PUSH(*dest, sub);
        sub = (SignalSub) SIGNAL_SUB_NEW();
    );

    return LUASTATUS_OK;

error:
    signal_sub_free(sub);
    destroy(pd);
    return LUASTATUS_ERR;
}

typedef struct {
    LuastatusPluginData *pd;
    LuastatusPluginRunFuncs funcs;
} PluginRunArgs;

static
void
callback_signal(
    GDBusConnection *connection,
    const gchar *sender_name,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *signal_name,
    GVariant *parameters,
    gpointer user_data)
{
    (void) connection;
    PluginRunArgs args = *(PluginRunArgs *) user_data;
    lua_State *L = args.funcs.call_begin(args.pd->userdata);

    lua_createtable(L, 0, 6); // L: table
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
    marshal(L, parameters); // L: table value
    lua_setfield(L, -2, "parameters"); // L: table

    args.funcs.call_end(args.pd->userdata);
}

static
gboolean
callback_timeout(gpointer user_data)
{
    PluginRunArgs args = *(PluginRunArgs *) user_data;
    lua_State *L = args.funcs.call_begin(args.pd->userdata);
    lua_createtable(L, 0, 1); // L: table
    lua_pushstring(L, "timeout"); // L: table string
    lua_setfield(L, -2, "what"); // L: table
    args.funcs.call_end(args.pd->userdata);
    return G_SOURCE_CONTINUE;
}

static
GDBusConnection *
maybe_connect_and_subscribe(GBusType bus_type, SubList sl, gpointer userdata, GError **err)
{
    if (!sl.size) {
        return NULL;
    }
    GDBusConnection *conn = g_bus_get_sync(bus_type, NULL, err);
    if (*err) {
        return NULL;
    }
    assert(conn);
    for (size_t i = 0; i < sl.size; ++i) {
        SignalSub s = sl.data[i];
        g_dbus_connection_signal_subscribe(
            conn,
            s.sender,
            s.interface,
            s.signal,
            s.object_path,
            s.arg0,
            s.flags,
            callback_signal,
            userdata,
            NULL);
    }
    return conn;
}

static
void
run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    GError *err = NULL;
    GDBusConnection *session_bus = NULL;
    GDBusConnection *system_bus = NULL;
    GMainLoop *mainloop = NULL;
    GMainContext *context = g_main_context_get_thread_default();
    GSource *source = NULL;

    PluginRunArgs userdata = {.pd = pd, .funcs = funcs};
    session_bus = maybe_connect_and_subscribe(G_BUS_TYPE_SESSION, p->session_subs, &userdata, &err);
    if (err) {
        LS_FATALF(pd, "cannot connect to the session bus: %s", err->message);
        goto error;
    }
    system_bus = maybe_connect_and_subscribe(G_BUS_TYPE_SYSTEM, p->system_subs, &userdata, &err);
    if (err) {
        LS_FATALF(pd, "cannot connect to the system bus: %s", err->message);
        goto error;
    }

    if (p->timeout_ms >= 0) {
        source = g_timeout_source_new(p->timeout_ms);
        g_source_set_callback(source, callback_timeout, &userdata, NULL);
        if (g_source_attach(source, context) == 0) {
            LS_FATALF(pd, "g_source_attach failed");
            goto error;
        }
    }
    mainloop = g_main_loop_new(context, FALSE);

    if (p->greet) {
        lua_State *L = funcs.call_begin(pd->userdata);
        lua_createtable(L, 0, 1); // L: table
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
    if (session_bus) {
        g_dbus_connection_close_sync(session_bus, NULL, NULL);
        g_object_unref(session_bus);
    }
    if (system_bus) {
        g_dbus_connection_close_sync(system_bus, NULL, NULL);
        g_object_unref(system_bus);
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
