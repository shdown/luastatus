/*
 * Copyright (C) 2015-2025  luastatus developers
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

#ifndef string_set_h_
#define string_set_h_

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    char **data;
    size_t size;
    size_t capacity;
} StringSet;

StringSet string_set_new(void);

// Clears and unfreezes the set.
void string_set_reset(StringSet *s);

void string_set_add(StringSet *s, const char *val);

void string_set_freeze(StringSet *s);

bool string_set_contains(StringSet s, const char *val);

void string_set_destroy(StringSet s);

#endif
