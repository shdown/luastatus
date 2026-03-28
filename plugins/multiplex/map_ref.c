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

#include "map_ref.h"
#include "external_context.h"
#include "libls/ls_panic.h"
#include "libls/ls_alloc_utils.h"
#include <stdlib.h>
#include <pthread.h>

typedef struct {
    pthread_mutex_t mtx;
    size_t nrefs;
} Control;

MapRef map_ref_new_empty(void)
{
    return (MapRef) {0};
}

static inline Control *load(MapRef ref)
{
    LS_ASSERT(ref.pptr != NULL);
    return *ref.pptr;
}

static inline void store(MapRef ref, Control *C)
{
    LS_ASSERT(ref.pptr != NULL);
    *ref.pptr = C;
}

void map_ref_init(MapRef *ref, ExternalContext ectx)
{
    ref->pptr = ectx->map_get(ectx->userdata, "plugin-init-mtx");

    Control *C = load(*ref);
    if (C) {
        ++C->nrefs;
    } else {
        C = LS_XNEW(Control, 1);
        LS_PTH_CHECK(pthread_mutex_init(&C->mtx, NULL));
        C->nrefs = 1;
        store(*ref, C);
    }
}

void map_ref_lock_mtx(MapRef ref)
{
    Control *C = load(ref);
    LS_PTH_CHECK(pthread_mutex_lock(&C->mtx));
}

void map_ref_unlock_mtx(MapRef ref)
{
    Control *C = load(ref);
    LS_PTH_CHECK(pthread_mutex_unlock(&C->mtx));
}

void map_ref_destroy(MapRef ref)
{
    if (!ref.pptr) {
        // Wasn't even initialized (with /map_ref_init()/).
        return;
    }

    Control *C = load(ref);

    // Since we currently own a reference, /C/ must be non-NULL and have a positive refcount.
    LS_ASSERT(C != NULL);
    LS_ASSERT(C->nrefs != 0);

    if (!--C->nrefs) {
        LS_PTH_CHECK(pthread_mutex_destroy(&C->mtx));
        free(C);
        store(ref, NULL);
    }
}
