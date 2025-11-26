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

#ifndef map_ref_h_
#define map_ref_h_

#include <stdlib.h>
#include <pthread.h>
#include "libls/ls_panic.h"
#include "libls/ls_alloc_utils.h"
#include "libls/ls_compdep.h"
#include "libls/ls_assert.h"
#include "external_context.h"

typedef struct {
    pthread_mutex_t mtx;
    size_t nrefs;
} MapValue_PI;

typedef struct {
    void **pptr;
} MapRef;

LS_INHEADER MapRef map_ref_new_empty(void)
{
    return (MapRef) {0};
}

LS_INHEADER MapValue_PI *map_ref_load(MapRef ref)
{
    LS_ASSERT(ref.pptr != NULL);
    return *ref.pptr;
}

LS_INHEADER MapValue_PI *map_ref_load_or_NULL(MapRef ref)
{
    if (ref.pptr == NULL) {
        return NULL;
    }
    return *ref.pptr;
}

LS_INHEADER void map_ref_store(MapRef ref, MapValue_PI *val)
{
    LS_ASSERT(ref.pptr != NULL);
    *ref.pptr = val;
}

LS_INHEADER void map_ref_init(MapRef *ref, ExternalContext ectx)
{
    ref->pptr = ectx->map_get(ectx->userdata, "plugin-init-mtx");

    MapValue_PI *val = map_ref_load(*ref);
    if (val) {
        ++val->nrefs;
    } else {
        val = LS_XNEW(MapValue_PI, 1);
        LS_PTH_CHECK(pthread_mutex_init(&val->mtx, NULL));
        val->nrefs = 1;
        map_ref_store(*ref, val);
    }
}

LS_INHEADER void map_ref_destroy(MapRef ref)
{
    MapValue_PI *val = map_ref_load_or_NULL(ref);
    if (val) {
        if (!--val->nrefs) {
            LS_PTH_CHECK(pthread_mutex_destroy(&val->mtx));
            free(val);
            map_ref_store(ref, NULL);
        }
    }
}

#endif
