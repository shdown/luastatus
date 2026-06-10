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

#include <stddef.h>
#include "libls/ls_compdep.h"

typedef struct {
    char *ptr;
    size_t len;
    size_t alloc;
    size_t keep_alloc;
} Buffer;

Buffer buffer_new(size_t keep_alloc);

void buffer_prepare(Buffer *B, size_t len);

LS_INHEADER char *buffer_ptr(Buffer *B)
{
    return B->ptr;
}

LS_INHEADER size_t buffer_len(Buffer *B)
{
    return B->len;
}

void buffer_free(Buffer *B);
