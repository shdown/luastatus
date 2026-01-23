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

#ifndef strlist_h_
#define strlist_h_

#include <stddef.h>

typedef struct {
    char **data;
    size_t size;
    size_t capacity;
    size_t max_size;
} Strlist;

Strlist strlist_new(size_t max_size);

int strlist_push(Strlist *x, const char *s);

int strlist_remove(Strlist *x, const char *s);

void strlist_destroy(Strlist x);

#endif
