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
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "libls/compdep.h"
#include "libls/alloc_utils.h"
#include "libls/string_.h"
#include "libls/panic.h"

// Concurrent queue.
// Instead of single value, it maintains a list of LS_String,
// each of which might get "updated" independently.
// The size of the list is limited to CONC_QUEUE_MAX_SLOTS.

typedef uint64_t CONC_QUEUE_MASK;

enum { CONC_QUEUE_MAX_SLOTS = sizeof(CONC_QUEUE_MASK) * 8 };

//MLC_PUSH_SCOPE("ConcQueue:decl")
typedef struct {
//MLC_DECL("mtx")
    pthread_mutex_t mtx;

//MLC_DECL("condvar")
    pthread_cond_t condvar;

//MLC_DECL("slots")
//MLC_DECL("slots[i]")
    LS_String *slots;
    size_t nslots;
    char slot_states[CONC_QUEUE_MAX_SLOTS];

    CONC_QUEUE_MASK new_data_mask;
} ConcQueue;
//MLC_POP_SCOPE()

//MLC_PUSH_SCOPE("ConcQueue:init")
LS_INHEADER void conc_queue_create(ConcQueue *q, size_t nslots)
{
    assert(nslots <= CONC_QUEUE_MAX_SLOTS);

//MLC_INIT("mtx")
    LS_PTH_CHECK(pthread_mutex_init(&q->mtx, NULL));

//MLC_INIT("condvar")
    LS_PTH_CHECK(pthread_cond_init(&q->condvar, NULL));

//MLC_INIT("slots")
    q->slots = LS_XNEW(LS_String, nslots);

//MLC_INIT("slots[i]")
    for (size_t i = 0; i < nslots; ++i) {
        q->slots[i] = ls_string_new_reserve(1024);
        q->slot_states[i] = 'z';
    }
    q->nslots = nslots;

    q->new_data_mask = 0;
}
//MLC_POP_SCOPE()

LS_INHEADER void conc_queue_update_slot(
    ConcQueue *q,
    size_t slot_idx,
    const char *buf, size_t nbuf,
    char state)
{
    LS_PTH_CHECK(pthread_mutex_lock(&q->mtx));

    LS_String *old = &q->slots[slot_idx];
    char old_state = q->slot_states[slot_idx];
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

//MLC_PUSH_SCOPE("ConcQueue:deinit")
LS_INHEADER void conc_queue_destroy(ConcQueue *q)
{
//MLC_DEINIT("mtx")
    LS_PTH_CHECK(pthread_mutex_destroy(&q->mtx));

//MLC_DEINIT("condvar")
    LS_PTH_CHECK(pthread_cond_destroy(&q->condvar));

//MLC_DEINIT("slots[i]")
    for (size_t i = 0; i < q->nslots; ++i) {
        ls_string_free(q->slots[i]);
    }

//MLC_DEINIT("slots")
    free(q->slots);
}
//MLC_POP_SCOPE()

#endif