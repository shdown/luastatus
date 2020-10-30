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
#include <glib.h>
#include <gio/gio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "libls/alloc_utils.h"
#include "libls/time_utils.h"

#include "libmoonvisit/moonvisit.h"

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "marshal.h"

typedef struct {
    char *sender;
    char *interface;
    char *signal;
    char *object_path;
    char *arg0;
    GDBusSignalFlags flags;
} Signal;

static void signal_free(Signal s)
{
    free(s.sender);
    free(s.interface);
    free(s.signal);
    free(s.object_path);
    free(s.arg0);
}

typedef struct {
    Signal *data;
    size_t size;
    size_t capacity;
} SignalList;

static inline SignalList signal_list_new(void)
{
    return (SignalList) {NULL, 0, 0};
}

static inline void signal_list_add(SignalList *x, Signal s)
{
    if (x->size == x->capacity) {
        x->data = ls_x2realloc(x->data, &x->capacity, sizeof(Signal));
    }
    x->data[x->size++] = s;
}

static inline void signal_list_destroy(SignalList *x)
{
    for (size_t i = 0; i < x->size; ++i) {
        signal_free(x->data[i]);
    }
    free(x->data);
}

enum {
    BUS_SESSION = 0,
    BUS_SYSTEM = 1,
};

typedef struct {
    SignalList subs[2];
    double tmo;
    bool greet;
} Priv;

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    for (int i = 0; i < 2; ++i) {
        signal_list_destroy(&p->subs[i]);
    }
    free(p);
}

static int parse_bus_str(MoonVisit *mv, void *ud, const char *s, size_t ns)
{
    (void) ns;
    int *out = ud;
    if (strcmp(s, "session") == 0) {
        *out = BUS_SESSION;
        return 1;
    }
    if (strcmp(s, "system") == 0) {
        *out = BUS_SYSTEM;
        return 1;
    }
    moon_visit_err(mv, "unknown bus: '%s'", s);
    return -1;
}

static int parse_flags_elem(MoonVisit *mv, void *ud, int kpos, int vpos)
{
    mv->where = "'signals' element, 'flags' element";
    (void) kpos;

    GDBusSignalFlags *out = ud;

    if (moon_visit_checktype_at(mv, "", vpos, LUA_TSTRING) < 0)
        goto error;

    const char *s = lua_tostring(mv->L, vpos);
    if (strcmp(s, "match_arg0_namespace") == 0) {
        *out |= G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_NAMESPACE;
        return 1;
    }
    if (strcmp(s, "match_arg0_path") == 0) {
        *out |= G_DBUS_SIGNAL_FLAGS_MATCH_ARG0_PATH;
        return 1;
    }
    moon_visit_err(mv, "unknown flag: '%s'", s);
error:
    return -1;
}

static int parse_signals_elem(MoonVisit *mv, void *ud, int kpos, int vpos)
{
    mv->where = "'signals' element";
    (void) kpos;

    Priv *p = ud;
    Signal s = {0};

    if (moon_visit_checktype_at(mv, "", vpos, LUA_TTABLE) < 0)
        goto error;

    int bus = BUS_SESSION;
    if (moon_visit_str_f(mv, vpos, "bus", parse_bus_str, &bus, true) < 0)
        goto error;

    if (moon_visit_str(mv, vpos, "sender", &s.sender, NULL, true) < 0)
        goto error;

    if (moon_visit_str(mv, vpos, "interface", &s.interface, NULL, true) < 0)
        goto error;

    if (moon_visit_str(mv, vpos, "signal", &s.signal, NULL, true) < 0)
        goto error;

    if (moon_visit_str(mv, vpos, "object_path", &s.object_path, NULL, true) < 0)
        goto error;

    if (moon_visit_str(mv, vpos, "arg0", &s.arg0, NULL, true) < 0)
        goto error;

    if (moon_visit_table_f(mv, vpos, "flags", parse_flags_elem, &s.flags, true) < 0)
        goto error;

    signal_list_add(&p->subs[bus], s);
    return 1;

error:
    signal_free(s);
    return -1;
}

static int init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .subs = {signal_list_new(), signal_list_new()},
        .tmo = -1,
        .greet = false,
    };
    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse greet
    if (moon_visit_bool(&mv, -1, "greet", &p->greet, true) < 0)
        goto mverror;

    // Parse timeout
    if (moon_visit_num(&mv, -1, "timeout", &p->tmo, true) < 0)
        goto mverror;

    // Parse signals
    if (moon_visit_table_f(&mv, -1, "signals", parse_signals_elem, p, false) < 0)
        goto mverror;

    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
//error:
    destroy(pd);
    return LUASTATUS_ERR;
}

typedef struct {
    LuastatusPluginData *pd;
    LuastatusPluginRunFuncs funcs;
} PluginRunArgs;

static void callback_signal(
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

static gboolean callback_timeout(gpointer user_data)
{
    PluginRunArgs args = *(PluginRunArgs *) user_data;
    lua_State *L = args.funcs.call_begin(args.pd->userdata);
    lua_createtable(L, 0, 1); // L: table
    lua_pushstring(L, "timeout"); // L: table string
    lua_setfield(L, -2, "what"); // L: table
    args.funcs.call_end(args.pd->userdata);
    return G_SOURCE_CONTINUE;
}

static GDBusConnection * maybe_connect_and_subscribe(
        GBusType bus_type,
        SignalList x,
        gpointer userdata,
        GError **err)
{
    if (!x.size)
        return NULL;

    GDBusConnection *conn = g_bus_get_sync(bus_type, NULL, err);
    if (*err)
        return NULL;

    assert(conn);
    for (size_t i = 0; i < x.size; ++i) {
        Signal s = x.data[i];
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

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    GError *err = NULL;
    GDBusConnection *session_bus = NULL;
    GDBusConnection *system_bus = NULL;
    GMainLoop *mainloop = NULL;
    GMainContext *context = g_main_context_get_thread_default();
    GSource *source = NULL;

    PluginRunArgs userdata = {.pd = pd, .funcs = funcs};

    session_bus = maybe_connect_and_subscribe(
        G_BUS_TYPE_SESSION, p->subs[BUS_SESSION], &userdata, &err);

    if (err) {
        LS_FATALF(pd, "cannot connect to the session bus: %s", err->message);
        goto error;
    }

    system_bus = maybe_connect_and_subscribe(
        G_BUS_TYPE_SYSTEM, p->subs[BUS_SYSTEM], &userdata, &err);

    if (err) {
        LS_FATALF(pd, "cannot connect to the system bus: %s", err->message);
        goto error;
    }

    if (p->tmo >= 0) {
        int tmo_ms = ls_tmo_to_ms(p->tmo);
        source = g_timeout_source_new(tmo_ms);
        g_source_set_callback(source, callback_timeout, &userdata, NULL);
        if (g_source_attach(source, context) == 0) {
            LS_FATALF(pd, "g_source_attach() failed");
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
