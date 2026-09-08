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

#include "vec.h"
#include <stdlib.h>
#include "common.h"

Vec vec_new(void)
{
    return (Vec) {0};
}

void vec_push(Vec *v, void *p)
{
    xassert(v);

    if (v->size == v->capacity) {
        v->data = x2realloc(v->data, &v->capacity, sizeof(void *));
    }
    v->data[v->size++] = p;
}

void *vec_get(Vec *v, size_t i)
{
    xassert(v);
    xassert(i < v->size);

    return v->data[i];
}

static inline int sort_compare(Vec *v, VecSortComparator cmp, size_t i, size_t j)
{
    xassert(i < v->size);
    xassert(j < v->size);

    return cmp(v->data[i], v->data[j]);
}

static inline void sort_swap(Vec *v, size_t i, size_t j)
{
    xassert(i < v->size);
    xassert(j < v->size);

    void *tmp = v->data[i];
    v->data[i] = v->data[j];
    v->data[j] = tmp;
}

void vec_sort(Vec *v, VecSortComparator cmp)
{
    xassert(v);

    // Insertion sort.
    for (size_t i = 1; i < v->size; ++i) {
        size_t j = i;
        while (j > 0 && sort_compare(v, cmp, j - 1, j) > 0) {
            sort_swap(v, j, j - 1);
            --j;
        }
    }
}

void vec_destroy(Vec *v)
{
    xassert(v);

    free(v->data);
}
