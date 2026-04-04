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
#include <string.h>
#include <stdlib.h>
#include "common.h"

void buffer_append(Buffer *b, const char *chunk, size_t len)
{
    if (!len) {
        return;
    }
    while (b->capacity - b->size < len) {
        b->data = x2realloc(b->data, &b->capacity, sizeof(char));
    }
    memcpy(b->data + b->size, chunk, len);
    b->size += len;
}

void buffer_destroy(Buffer *b)
{
    free(b->data);
}
