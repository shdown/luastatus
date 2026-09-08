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

typedef struct {
    void **data;
    size_t size;
    size_t capacity;
} Vec;

Vec vec_new(void);

void vec_push(Vec *v, void *p);

void *vec_get(Vec *v, size_t i);

typedef int (*VecSortComparator)(void *, void *);

void vec_sort(Vec *v, VecSortComparator cmp);

void vec_destroy(Vec *v);
