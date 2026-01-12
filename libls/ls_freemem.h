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

#ifndef ls_freemem_h_
#define ls_freemem_h_

#include <stddef.h>
#include "ls_alloc_utils.h"
#include "ls_compdep.h"
#include "ls_assert.h"

// This function is called whenever a dynamic array is cleared.
// We want to "preserve" at most 1 Kb of the previous capacity.
LS_INHEADER LS_ATTR_WARN_UNUSED
void *ls_freemem(void *data, size_t *psize, size_t *pcapacity, size_t elemsz)
{
    LS_ASSERT(elemsz != 0);
    size_t new_capacity = 1024 / elemsz;
    if (*pcapacity > new_capacity) {
        data = ls_xrealloc(data, new_capacity, elemsz);
        *pcapacity = new_capacity;
    }
    *psize = 0;
    return data;
}

#define LS_M_FREEMEM(Data_, SizePtr_, CapacityPtr_) \
    ((__typeof__(Data_)) ls_freemem( \
        (Data_),        \
        (SizePtr_),     \
        (CapacityPtr_), \
        sizeof(*(Data_))))

#endif
