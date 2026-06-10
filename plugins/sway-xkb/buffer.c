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

#include "buffer.h"
#include <stdlib.h>
#include <stdint.h>
#include "libls/ls_panic.h"
#include "libls/ls_alloc_utils.h"

Buffer buffer_new(size_t keep_alloc)
{
    LS_ASSERT(keep_alloc > 0);

    return (Buffer) {
        .ptr = LS_XNEW(char, keep_alloc),
        .len = 0,
        .alloc = keep_alloc,
        .keep_alloc = keep_alloc,
    };
}

void buffer_prepare(Buffer *B, size_t len)
{
    if (len == SIZE_MAX) {
        ls_oom();
    }

    size_t new_alloc = len + 1;
    if (new_alloc < B->keep_alloc) {
        new_alloc = B->keep_alloc;
    }

    if (new_alloc != B->alloc) {
        B->ptr = LS_M_XREALLOC(B->ptr, new_alloc);
        B->alloc = new_alloc;
    }

    B->len = len;
    B->ptr[len] = '\0';
}

void buffer_free(Buffer *B)
{
    free(B->ptr);
}
