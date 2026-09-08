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

#include <glib.h>
#include <stdbool.h>

struct WonderfulServer;
typedef struct WonderfulServer WonderfulServer;

typedef void (*Wonderful_OnRunningCallback)(void *ud);

WonderfulServer *wonderful_server_new(
    const char *bus_name,
    const char *interface_name,
    const char *object_path,
    void *ud,
    Wonderful_OnRunningCallback on_running_callback
);

// Returns NULL on error.
typedef GVariant *(*Wonderful_MethodCallback)(void *ud, GVariant *params);

void wonderful_server_add_method(
    WonderfulServer *x,
    const char *name,
    const char *in_signature,
    const char *out_signature,
    Wonderful_MethodCallback callback
);

// Returns NULL on error.
typedef GVariant *(*Wonderful_PropertyCallbackGet)(void *ud);

typedef bool (*Wonderful_PropertyCallbackSet)(void *ud, GVariant *value);

void wonderful_server_add_property(
    WonderfulServer *x,
    const char *property,
    const char *type_signature,
    Wonderful_PropertyCallbackGet callback_get,
    Wonderful_PropertyCallbackSet callback_set
);

void wonderful_server_run(WonderfulServer *x);

void wonderful_server_destroy(WonderfulServer *x);
