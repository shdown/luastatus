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

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <glib.h>
#include "common.h"
#include "wonderful_server.h"
#include "vec.h"

#define MY_NAME "dbus_serv"

static char *my_property_value;
static FILE *my_fifo;

#define log(...) log_impl(__func__, __VA_ARGS__)

ATTR_PRINTF(2, 3)
static void log_impl(const char *func, const char *fmt, ...)
{
    fprintf(stderr, "[%s] <%s> ", MY_NAME, func);

    va_list vl;
    va_start(vl, fmt);

    vfprintf(stderr, fmt, vl);

    va_end(vl);
}

static void on_running_callback(void *ud)
{
    (void) ud;

    xassert(my_fifo != NULL);

    log("Up and runnning. Writing to FIFO...\n");

    fputs("running\n", my_fifo);
    fflush(my_fifo);
    if (ferror(my_fifo)) {
        xpanicf("cannot write to FIFO\n");
    }

    log("OK, written to FIFO\n");
}

static GVariant *PROP_GET_my_property(void *ud)
{
    (void) ud;

    if (!my_property_value) {
        my_property_value = xstrdup("");
    }

    log("current value = '%s'\n", my_property_value);

    return g_variant_new("s", my_property_value);
}

static bool PROP_SET_my_property(void *ud, GVariant *value)
{
    (void) ud;

    free(my_property_value);

    const gchar *arg;
    g_variant_get(value, "&s", &arg);

    my_property_value = xstrdup(arg);

    log("new value = '%s'\n", my_property_value);

    return true;
}

static GVariant *MTH_upcase(void *ud, GVariant *params)
{
    (void) ud;

    const gchar *arg;
    g_variant_get(params, "(&s)", &arg);

    log("called on '%s'\n", arg);

    // We only care about ASCII.
    gchar *uppercased = g_ascii_strup(arg, -1);
    GVariant *res = g_variant_new("(s)", uppercased);
    g_free(uppercased);

    return res;
}

static GVariant *MTH_return_forty_two(void *ud, GVariant *params)
{
    (void) ud;
    (void) params;

    log("called\n");

    return g_variant_new("(i)", (gint32) 42);
}

static gchar *hexify(const gchar *s)
{
    GString *res = g_string_new("");
    for (const gchar *cur = s; *cur; ++cur) {
        g_string_append_printf(res, "%02X", (unsigned) (uint8_t) *cur);
    }
    return g_string_free_and_steal(res);
}

static GVariant *MTH_convert_array_to_dict_hexify(void *ud, GVariant *params)
{
    (void) ud;

    log("called (see below)\n");

    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE("a{ss}"));

    GVariant *arr = g_variant_get_child_value(params, 0);

    GVariantIter iter;
    g_variant_iter_init(&iter, arr);

    const gchar *s;
    size_t i = 0;
    while (g_variant_iter_next(&iter, "&s", &s)) {

        log("array elem [%zu] = '%s'\n", i, s);

        char i_buf[32];
        snprintf(i_buf, sizeof(i_buf), "%zu", i);

        gchar *k = hexify(i_buf);
        gchar *v = hexify(s);

        log("adding dict entry: '%s' => '%s'\n", k, v);

        g_variant_builder_add(&builder, "{ss}", k, v);

        g_free(k);
        g_free(v);

        ++i;
    }

    log("total # of elements: %zu\n", i);

    g_variant_unref(arr);

    return g_variant_new("(@a{ss})", g_variant_builder_end(&builder));
}

typedef struct {
    char *k;
    char *v;
} KvEntry;

static int my_kv_entry_cmp(void *p, void *q)
{
    KvEntry *a = p;
    KvEntry *b = q;
    int res = strcmp(a->k, b->k);
    if (res != 0) {
        return res;
    }
    return strcmp(a->v, b->v);
}

static GVariant *MTH_convert_dict_to_string(void *ud, GVariant *params)
{
    (void) ud;

    GVariant *dict = g_variant_get_child_value(params, 0);

    GVariantIter iter;
    g_variant_iter_init(&iter, dict);

    Vec entries = vec_new();

    const gchar *k;
    const gchar *v;
    while (g_variant_iter_loop(&iter, "{&s&s}", &k, &v)) {
        KvEntry *entry = xmalloc(sizeof(KvEntry), 1);
        *entry = (KvEntry) {
            .k = xstrdup(k),
            .v = xstrdup(v),
        };
        vec_push(&entries, entry);
    }

    vec_sort(&entries, my_kv_entry_cmp);

    GString *res_gs = g_string_new("");
    for (size_t i = 0; i < entries.size; ++i) {
        if (i > 0) {
            g_string_append_c(res_gs, ',');
        }
        KvEntry *entry = entries.data[i];
        g_string_append_printf(res_gs, "%s:%s", entry->k, entry->v);

        free(entry->k);
        free(entry->v);
        free(entry);
    }
    vec_destroy(&entries);

    gchar *res_s = g_string_free_and_steal(res_gs);

    GVariant *res = g_variant_new("(s)", res_s);
    g_free(res_s);

    g_variant_unref(dict);

    return res;
}

static GVariant *concat_strings_of_tuple(GVariant *tuple, size_t nelems)
{
    xassert(g_variant_n_children(tuple) == nelems);

    GString *res_gs = g_string_new("");

    for (size_t i = 0; i < nelems; ++i) {
        GVariant *elem = g_variant_get_child_value(tuple, i);

        const gchar *s;
        g_variant_get(elem, "&s", &s);
        g_string_append(res_gs, s);

        g_variant_unref(elem);
    }

    gchar *res_s = g_string_free_and_steal(res_gs);

    log("result = '%s'\n", res_s);

    GVariant *res = g_variant_new("(s)", res_s);
    g_free(res_s);

    return res;
}

static GVariant *MTH_recv_tuple_0(void *ud, GVariant *params)
{
    (void) ud;
    (void) params;

    log("called\n");

    return g_variant_new("(s)", "Empty");
}

static GVariant *MTH_recv_tuple_1(void *ud, GVariant *params)
{
    (void) ud;

    log("called (see below)\n");

    return concat_strings_of_tuple(params, 1);
}

static GVariant *MTH_recv_tuple_2(void *ud, GVariant *params)
{
    (void) ud;

    log("called (see below)\n");

    return concat_strings_of_tuple(params, 2);
}

static GVariant *MTH_recv_tuple_3(void *ud, GVariant *params)
{
    (void) ud;

    log("called (see below)\n");

    return concat_strings_of_tuple(params, 3);
}

static const char *gvariant_class_as_string(GVariant *v)
{
    switch (g_variant_classify(v)) {

    case G_VARIANT_CLASS_BOOLEAN: return "bool";

    case G_VARIANT_CLASS_BYTE: return "byte";

    case G_VARIANT_CLASS_INT16: return "i16";
    case G_VARIANT_CLASS_UINT16: return "u16";

    case G_VARIANT_CLASS_INT32: return "i32";
    case G_VARIANT_CLASS_UINT32: return "u32";

    case G_VARIANT_CLASS_INT64: return "i64";
    case G_VARIANT_CLASS_UINT64: return "u64";

    case G_VARIANT_CLASS_DOUBLE: return "double";

    case G_VARIANT_CLASS_STRING: return "string";
    case G_VARIANT_CLASS_OBJECT_PATH: return "object_path";
    case G_VARIANT_CLASS_SIGNATURE: return "signature";

    case G_VARIANT_CLASS_VARIANT: return "variant";

    case G_VARIANT_CLASS_ARRAY: return "array";
    case G_VARIANT_CLASS_TUPLE: return "tuple";
    case G_VARIANT_CLASS_DICT_ENTRY: return "dict_entry";

    case G_VARIANT_CLASS_HANDLE: return "handle";

    default: return NULL;
    }
}

static GVariant *MTH_recv_variant(void *ud, GVariant *params)
{
    (void) ud;

    log("called (see below)\n");

    GVariant *boxed = g_variant_get_child_value(params, 0);
    GVariant *x = g_variant_get_variant(boxed);

    const char *result = gvariant_class_as_string(x);
    if (!result) {
        log("unknown class!\n");
        return NULL;
    }

    log("result = '%s'\n", result);

    g_variant_unref(x);
    g_variant_unref(boxed);

    return g_variant_new("(s)", result);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <PATH_TO_FIFO>\n", MY_NAME);
        return 2;
    }

    const char *fifo_path = argv[1];

    log("opening FIFO ('%s')...\n", fifo_path);
    my_fifo = fopen(fifo_path, "w");
    if (!my_fifo) {
        perror(fifo_path);
        return 1;
    }

    log("OK, opened FIFO\n");

    WonderfulServer *S = wonderful_server_new(
        "io.github.shdown.luastatus.test",
        "io.github.shdown.luastatus.test",
        "/io/github/shdown/luastatus/test/MyObject",
        NULL,
        on_running_callback
    );

    typedef struct {
        const char *name;
        const char *in_signature;
        const char *out_signature;
        Wonderful_MethodCallback callback;
    } MyMethod;

    static const MyMethod MY_METHODS[] = {
        {"Upcase", "(s)", "(s)", MTH_upcase},
        {"ReturnFortyTwo", "()", "(i)", MTH_return_forty_two},
        {"ConvertArrayToDictHexify", "(as)", "(a{ss})", MTH_convert_array_to_dict_hexify},
        {"ConvertDictToString", "(a{ss})", "(s)", MTH_convert_dict_to_string},

        {"RecvTuple0", "()", "(s)", MTH_recv_tuple_0},
        {"RecvTuple1", "(s)", "(s)", MTH_recv_tuple_1},
        {"RecvTuple2", "(ss)", "(s)", MTH_recv_tuple_2},
        {"RecvTuple3", "(sss)", "(s)", MTH_recv_tuple_3},

        {"RecvVariant", "(v)", "(s)", MTH_recv_variant},

        {0},
    };

    for (const MyMethod *cur = MY_METHODS; cur->name; ++cur) {
        wonderful_server_add_method(
            S,
            cur->name,
            cur->in_signature,
            cur->out_signature,
            cur->callback);
    }

    wonderful_server_add_property(
        S,
        "MyProperty",
        "s",
        PROP_GET_my_property,
        PROP_SET_my_property);

    wonderful_server_run(S);
    wonderful_server_destroy(S);

    return 40;
}
