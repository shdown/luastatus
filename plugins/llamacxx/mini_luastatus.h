/*
 * Copyright (C) 2025  luastatus developers
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

#ifndef mini_luastatus_h_
#define mini_luastatus_h_

#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include "conc_queue.h"
#include "external_context.h"

bool mini_luastatus_run(
    const char *lua_program,
    const char *name,
    ConcQueue *q,
    size_t q_idx,
    ExternalContext ectx,
    pthread_t *out_pid);

#endif