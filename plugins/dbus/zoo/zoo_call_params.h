#pragma once

#include <lua.h>
#include <glib.h>
#include <gio/gio.h>
#include <stdbool.h>

typedef struct {
    const char *key;
    char *value;
    bool nullable;
} Zoo_StrField;

typedef struct {
    Zoo_StrField *str_fields;

    const char *gvalue_field_name;
    bool gvalue_field_must_be_tuple;
    GVariant *gvalue;

    GBusType bus_type;
    GDBusCallFlags flags;
    int tmo_ms;
} Zoo_CallParams;

void zoo_call_params_parse(lua_State *L, Zoo_CallParams *p, int arg);

void zoo_call_params_free(Zoo_CallParams *p);
