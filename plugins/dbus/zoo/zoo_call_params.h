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
