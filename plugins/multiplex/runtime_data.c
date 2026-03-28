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

#include "runtime_data.h"
#include "conq.h"
#include "external_context.h"
#include "runner.h"
#include "libls/ls_panic.h"
#include "libls/ls_alloc_utils.h"

void runtime_data_init(RuntimeData *x, ExternalContext ectx, size_t n)
{
    LS_ASSERT(n != 0);

    x->runners = LS_XNEW(Runner *, n);
    for (size_t i = 0; i < n; ++i) {
        x->runners[i] = NULL;
    }

    x->cq = conq_create(n);

    x->bufs = LS_XNEW(LS_String, n);
    for (size_t i = 0; i < n; ++i) {
        x->bufs[i] = ls_string_new_reserve(512);
    }

    x->states = LS_XNEW(ConqSlotState, n);
    for (size_t i = 0; i < n; ++i) {
        x->states[i] = CONQ_SLOT_STATE_EMPTY;
    }

    x->u_uds = LS_XNEW(UniversalUserdata, n);
    for (size_t i = 0; i < n; ++i) {
        x->u_uds[i] = (UniversalUserdata) {
            .ectx = ectx,
            .i = i,
        };
    }

    x->ectx = ectx;
}
