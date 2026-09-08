/*
 * Copyright (C) 2026  luastatus developers
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

#include "wonderful_server.h"
#include <glib.h>
#include <gio/gio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "vec.h"
#include "my_glib_compat.h"

// NOTE: we paste strings into XML as they are, because it's a small testing program;
// malicious/invalid names of methods/properties are impossible.

typedef struct {
    char *name;
    GVariantType *in;
    GVariantType *out;
    Wonderful_MethodCallback callback;
} Method;

static void method_destroy_inner(const Method *M)
{
    free(M->name);
    g_variant_type_free(M->in);
    g_variant_type_free(M->out);
}

typedef struct {
    char *name;
    char *type_signature;
    Wonderful_PropertyCallbackGet callback_get;
    Wonderful_PropertyCallbackSet callback_set;
} Property;

static void property_destroy_inner(const Property *P)
{
    free(P->name);
    free(P->type_signature);
}

struct WonderfulServer {
    char *bus_name;
    char *interface_name;
    char *object_path;
    void *ud;
    Vec methods;
    Vec properties;
    Wonderful_OnRunningCallback on_running_callback;

    GMainLoop *loop;
    GDBusNodeInfo *node_info;
    GDBusConnection *cnx;
    guint owner_id;
    guint reg_id;
};

WonderfulServer *wonderful_server_new(
    const char *bus_name,
    const char *interface_name,
    const char *object_path,
    void *ud,
    Wonderful_OnRunningCallback on_running_callback)
{
    xassert(bus_name != NULL);
    xassert(interface_name != NULL);
    xassert(object_path != NULL);

    WonderfulServer *x = xmalloc(sizeof(WonderfulServer), 1);
    *x = (WonderfulServer) {
        .bus_name = xstrdup(bus_name),
        .interface_name = xstrdup(interface_name),
        .object_path = xstrdup(object_path),
        .ud = ud,
        .methods = vec_new(),
        .properties = vec_new(),
        .on_running_callback = on_running_callback,

        .loop = NULL,
        .node_info = NULL,
        .cnx = NULL,
        .owner_id = 0,
        .reg_id = 0,
    };
    return x;
}

static GVariantType *parse_signature_of_tuple(const char *signature)
{
    xassert(signature != NULL);

    if (!g_variant_type_string_is_valid(signature)) {
        xpanicf("Invalid signature '%s'.\n", signature);
    }
    GVariantType *r = g_variant_type_new(signature);
    if (!g_variant_type_is_tuple(r)) {
        xpanicf("Signature '%s' does not represent a tuple.\n", signature);
    }
    return r;
}

void wonderful_server_add_method(
    WonderfulServer *x,
    const char *name,
    const char *in_signature,
    const char *out_signature,
    Wonderful_MethodCallback callback)
{
    xassert(x != NULL);
    xassert(name != NULL);
    xassert(in_signature != NULL);
    xassert(out_signature != NULL);
    xassert(callback != NULL);

    GVariantType *in = parse_signature_of_tuple(in_signature);
    GVariantType *out = parse_signature_of_tuple(out_signature);

    Method *M = xmalloc(sizeof(Method), 1);
    *M = (Method) {
        .name = xstrdup(name),
        .in = in,
        .out = out,
        .callback = callback,
    };
    vec_push(&x->methods, M);
}

void wonderful_server_add_property(
    WonderfulServer *x,
    const char *name,
    const char *type_signature,
    Wonderful_PropertyCallbackGet callback_get,
    Wonderful_PropertyCallbackSet callback_set)
{
    xassert(x != NULL);
    xassert(name != NULL);
    xassert(type_signature != NULL);
    xassert(callback_get != NULL);
    xassert(callback_set != NULL);

    if (!g_variant_type_string_is_valid(type_signature)) {
        xpanicf("Invalid signature '%s'.\n", type_signature);
    }

    Property *P = xmalloc(sizeof(Property), 1);
    *P = (Property) {
        .name = xstrdup(name),
        .type_signature = xstrdup(type_signature),
        .callback_get = callback_get,
        .callback_set = callback_set,
    };
    vec_push(&x->properties, P);
}

static void add_method_args(
        GString *GS,
        const GVariantType *tuple,
        const char *direction)
{
    size_t i = 0;
    for (
        const GVariantType *t = g_variant_type_first(tuple);
        t != NULL;
        t = g_variant_type_next(t), ++i)
    {
        char *type_sig = g_variant_type_dup_string(t);
        g_string_append_printf(
            GS,
            "<arg type='%s' name='i%zu' direction='%s' />\n",
            type_sig,
            i,
            direction
        );
        g_free(type_sig);
    }
}

static void add_method(GString *GS, Method *M)
{
    g_string_append_printf(GS, "<method name='%s'>\n", M->name);

    add_method_args(GS, M->in, "in");
    add_method_args(GS, M->out, "out");

    g_string_append_printf(GS, "</method>\n");
}

static gchar *build_xml(WonderfulServer *x)
{
    GString *GS = g_string_new("");

    g_string_append_printf(GS, "<node>\n");
    g_string_append_printf(GS, "<interface name='%s'>\n", x->interface_name);

    for (size_t i = 0; i < x->methods.size; ++i) {
        Method *M = vec_get(&x->methods, i);
        add_method(GS, M);
    }

    for (size_t i = 0; i < x->properties.size; ++i) {
        Property *P = vec_get(&x->properties, i);
        g_string_append_printf(
            GS,
            "<property type='%s' name='%s' access='readwrite' />\n",
            P->type_signature,
            P->name
        );
    }

    g_string_append_printf(GS, "</interface>\n");
    g_string_append_printf(GS, "</node>\n");

    return my_g_string_free_and_steal(GS);
}

static void die_if_gerror_is_set(const char *where, GError *e)
{
    if (e) {
        xpanicf("%s: glib error: %s\n", where, e->message);
    }
}

static Method *find_method(WonderfulServer *x, const char *name)
{
    for (size_t i = 0; i < x->methods.size; ++i) {
        Method *M = vec_get(&x->methods, i);
        if (strcmp(M->name, name) == 0) {
            return M;
        }
    }
    return NULL;
}

static Property *find_property(WonderfulServer *x, const char *name)
{
    for (size_t i = 0; i < x->properties.size; ++i) {
        Property *P = vec_get(&x->properties, i);
        if (strcmp(P->name, name) == 0) {
            return P;
        }
    }
    return NULL;
}

static void handle_method_call(
    GDBusConnection *cnx,
    const gchar *sender,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *method_name,
    GVariant *params,
    GDBusMethodInvocation *invc,
    gpointer gptr_ud)
{
    (void) cnx;
    (void) sender;
    (void) object_path;
    (void) interface_name;

    WonderfulServer *x = gptr_ud;

    Method *M = find_method(x, method_name);
    if (!M) {
        g_dbus_method_invocation_return_error(
            invc,
            G_DBUS_ERROR,
            G_DBUS_ERROR_UNKNOWN_METHOD,
            "method '%s' not found",
            method_name
        );
        return;
    }

    GVariant *result = M->callback(x->ud, params);
    if (!result) {
        g_dbus_method_invocation_return_error(
            invc,
            G_DBUS_ERROR,
            G_DBUS_ERROR_FAILED,
            "call of method '%s' failed",
            method_name
        );
        return;
    }

    g_dbus_method_invocation_return_value(invc, result);
}

// Note: throwing glib errors from get-property/set-property callbacks is non-trivial.
// So let's just panic instead.

static GVariant *handle_get_property(
    GDBusConnection *cnx,
    const gchar *sender,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *property_name,
    GError **error,
    gpointer gptr_ud)
{
    (void) cnx;
    (void) sender;
    (void) object_path;
    (void) interface_name;
    (void) error;

    WonderfulServer *x = gptr_ud;

    Property *P = find_property(x, property_name);
    if (!P) {
        xpanicf("cannot find property '%s'\n", property_name);
    }

    GVariant *result = P->callback_get(x->ud);
    if (!result) {
        xpanicf("callback_get for property '%s' reported failure", property_name);
    }

    return result;
}

static gboolean handle_set_property(
    GDBusConnection *cnx,
    const gchar *sender,
    const gchar *object_path,
    const gchar *interface_name,
    const gchar *property_name,
    GVariant *value,
    GError **error,
    gpointer gptr_ud)
{
    (void) cnx;
    (void) sender;
    (void) object_path;
    (void) interface_name;
    (void) error;

    WonderfulServer *x = gptr_ud;

    Property *P = find_property(x, property_name);
    if (!P) {
        xpanicf("cannot find property '%s'\n", property_name);
    }

    if (!P->callback_set(x->ud, value)) {
        xpanicf("callback_set for property '%s' reported failure", property_name);
    }

    return TRUE;
}

static const GDBusInterfaceVTable method_vtable = {
    .method_call = handle_method_call,
    .get_property = handle_get_property,
    .set_property = handle_set_property,
};

static void on_bus_acquired(
    GDBusConnection *cnx,
    const gchar *name,
    gpointer gptr_ud)
{
    (void) name;

    WonderfulServer *x = gptr_ud;
    x->cnx = cnx;

    GDBusInterfaceInfo *iface = x->node_info->interfaces[0];
    GError *err = NULL;
    guint reg_id = g_dbus_connection_register_object(
        cnx,
        x->object_path,
        iface,
        &method_vtable,
        x,
        NULL,
        &err
    );
    die_if_gerror_is_set("g_dbus_connection_register_object", err);

    xassert(reg_id != 0);
    x->reg_id = reg_id;
}

static void on_name_acquired(GDBusConnection *cnx, const gchar *name, gpointer gptr_ud)
{
    (void) cnx;
    (void) name;

    WonderfulServer *x = gptr_ud;

    if (x->on_running_callback) {
        x->on_running_callback(x->ud);
    }
}

void wonderful_server_run(WonderfulServer *x)
{
    xassert(x != NULL);

    gchar *xml = build_xml(x);
    GError *err = NULL;

    x->node_info = g_dbus_node_info_new_for_xml(xml, &err);
    die_if_gerror_is_set("g_dbus_node_info_new_for_xml", err);

    x->owner_id = g_bus_own_name(
        G_BUS_TYPE_SESSION,
        x->bus_name,
        G_BUS_NAME_OWNER_FLAGS_NONE,
        on_bus_acquired,
        on_name_acquired,
        /*on_name_lost=*/NULL,
        x,
        NULL
    );

    xassert(x->owner_id != 0);

    x->loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(x->loop);

    g_free(xml);
}

void wonderful_server_destroy(WonderfulServer *x)
{
    xassert(x != NULL);

    if (x->loop) {
        g_main_loop_unref(x->loop);
    }

    if (x->cnx && x->reg_id) {
        g_dbus_connection_unregister_object(x->cnx, x->reg_id);
    }

    if (x->owner_id) {
        g_bus_unown_name(x->owner_id);
    }

    if (x->node_info) {
        g_dbus_node_info_unref(x->node_info);
    }

    for (size_t i = 0; i < x->methods.size; ++i) {
        Method *M = vec_get(&x->methods, i);
        method_destroy_inner(M);
        free(M);
    }
    vec_destroy(&x->methods);

    for (size_t i = 0; i < x->properties.size; ++i) {
        Property *P = vec_get(&x->properties, i);
        property_destroy_inner(P);
        free(P);
    }
    vec_destroy(&x->properties);

    free(x->bus_name);
    free(x->interface_name);
    free(x->object_path);
    free(x);
}
