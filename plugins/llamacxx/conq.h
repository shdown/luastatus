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

#ifndef conq_h_
#define conq_h_

#include <stdint.h>
#include <stdlib.h>
#include "libls/ls_string.h"

// Concurrent queue.
// Instead of single value, it maintains a list of /LS_String/s,
// each of which might get "updated" independently.
// The size of the list is limited to /CONQ_MAX_SLOTS/.

typedef uint64_t CONQ_MASK;

enum { CONQ_MAX_SLOTS = sizeof(CONQ_MASK) * 8 };

typedef enum {
    CONQ_SLOT_STATE_EMPTY,
    CONQ_SLOT_STATE_NIL,
    CONQ_SLOT_STATE_HAS_VALUE,
    CONQ_SLOT_STATE_ERROR_PLUGIN_DONE,
    CONQ_SLOT_STATE_ERROR_LUA_ERR,
} ConqSlotState;

struct Conq;
typedef struct Conq Conq;

Conq *conq_create(size_t nslots);

void conq_update_slot(
    Conq *q,
    size_t slot_idx,
    const char *buf, size_t nbuf,
    ConqSlotState state);

CONQ_MASK conq_fetch_updates(
    Conq *q,
    LS_String *out,
    ConqSlotState *out_states);

CONQ_MASK conq_peek_at_updates(Conq *q);

void conq_destroy(Conq *q);

#endif
