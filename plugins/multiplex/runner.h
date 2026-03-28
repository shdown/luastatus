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
#include "external_context.h"

typedef enum {
    REASON_NEW_DATA,
    REASON_LUA_ERROR,
    REASON_PLUGIN_EXITED,
} RunnerCallbackReason;

typedef void (*RunnerCallback)(
    void *ud,
    RunnerCallbackReason reason,
    lua_State *L);

typedef enum {
    EVT_RESULT_OK,
    EVT_RESULT_LUA_ERROR,
    EVT_RESULT_NO_HANDLER,
} RunnerEventResult;

struct Runner;
typedef struct Runner Runner;

Runner *runner_new(
    const char *name,
    const char *code,
    ExternalContext ectx,
    RunnerCallback callback,
    void *callback_ud);

void runner_run(Runner *R);

lua_State *runner_event_begin(Runner *R);

RunnerEventResult runner_event_end(Runner *R);

void runner_destroy(Runner *R);
