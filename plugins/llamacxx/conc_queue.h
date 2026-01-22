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

#ifndef conc_queue_h_
#define conc_queue_h_

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include "libls/ls_compdep.h"
#include "libls/ls_alloc_utils.h"
#include "libls/ls_string.h"
#include "libls/ls_panic.h"
#include "libls/ls_assert.h"

// Concurrent queue.
// Instead of single value, it maintains a list of LS_String,
// each of which might get "updated" independently.
// The size of the list is limited to CONC_QUEUE_MAX_SLOTS.

typedef uint64_t CONC_QUEUE_MASK;

enum { CONC_QUEUE_MAX_SLOTS = sizeof(CONC_QUEUE_MASK) * 8 };

typedef enum {
    SLOT_STATE_EMPTY,
    SLOT_STATE_NIL,
    SLOT_STATE_HAS_VALUE,
    SLOT_STATE_ERROR_PLUGIN_DONE,
    SLOT_STATE_ERROR_LUA_ERR,
} SlotState;

typedef struct {
    pthread_mutex_t mtx;
    pthread_cond_t condvar;

    LS_String *slots;
    size_t nslots;

    char slot_states[CONC_QUEUE_MAX_SLOTS];

    CONC_QUEUE_MASK new_data_mask;
} ConcQueue;

LS_INHEADER void conc_queue_create(ConcQueue *q, size_t nslots)
{
    LS_ASSERT(nslots <= CONC_QUEUE_MAX_SLOTS);

    LS_PTH_CHECK(pthread_mutex_init(&q->mtx, NULL));
    LS_PTH_CHECK(pthread_cond_init(&q->condvar, NULL));

    q->slots = LS_XNEW(LS_String, nslots);
    for (size_t i = 0; i < nslots; ++i) {
        q->slots[i] = ls_string_new_reserve(1024);
        q->slot_states[i] = SLOT_STATE_EMPTY;
    }
    q->nslots = nslots;

    q->new_data_mask = 0;
}

LS_INHEADER void conc_queue_update_slot(
    ConcQueue *q,
    size_t slot_idx,
    const char *buf, size_t nbuf,
    SlotState state)
{
    LS_ASSERT(slot_idx < q->nslots);

    LS_PTH_CHECK(pthread_mutex_lock(&q->mtx));

    LS_String *old = &q->slots[slot_idx];
    SlotState old_state = q->slot_states[slot_idx];
    if (ls_string_eq_b(*old, buf, nbuf) && old_state == state) {
        goto unlock;
    }
    ls_string_assign_b(old, buf, nbuf);
    q->slot_states[slot_idx] = state;

    q->new_data_mask |= ((CONC_QUEUE_MASK) 1) << slot_idx;
    LS_PTH_CHECK(pthread_cond_signal(&q->condvar));

unlock:
    LS_PTH_CHECK(pthread_mutex_unlock(&q->mtx));
}

LS_INHEADER CONC_QUEUE_MASK conc_queue_fetch_updates(
    ConcQueue *q,
    LS_String *out,
    char *out_states)
{
    LS_PTH_CHECK(pthread_mutex_lock(&q->mtx));

    CONC_QUEUE_MASK mask;
    while (!(mask = q->new_data_mask)) {
        LS_PTH_CHECK(pthread_cond_wait(&q->condvar, &q->mtx));
    }

    for (size_t i = 0; i < q->nslots; ++i) {
        if ((mask >> i) & 1) {
            LS_String slot = q->slots[i];
            ls_string_assign_b(&out[i], slot.data, slot.size);
            out_states[i] = q->slot_states[i];
        }
    }

    q->new_data_mask = 0;

    LS_PTH_CHECK(pthread_mutex_unlock(&q->mtx));

    return mask;
}

LS_INHEADER CONC_QUEUE_MASK conc_queue_peek_at_updates(ConcQueue *q)
{
    LS_PTH_CHECK(pthread_mutex_lock(&q->mtx));

    CONC_QUEUE_MASK res = q->new_data_mask;

    LS_PTH_CHECK(pthread_mutex_unlock(&q->mtx));

    return res;
}

LS_INHEADER void conc_queue_destroy(ConcQueue *q)
{
    LS_PTH_CHECK(pthread_mutex_destroy(&q->mtx));
    LS_PTH_CHECK(pthread_cond_destroy(&q->condvar));

    for (size_t i = 0; i < q->nslots; ++i) {
        ls_string_free(q->slots[i]);
    }

    free(q->slots);
}

#endif
