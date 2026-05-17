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

#include "strset_mergesort.h"
#include <stdlib.h>
#include <string.h>
#include "libls/ls_alloc_utils.h"

// See file 'DEV_NOTES.txt' for why we roll our own sorting/binsearch
// implementation.

static inline void copy(char **dst, char **src, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        dst[i] = src[i];
    }
}

void strset_mergesort(char **data, size_t ndata)
{
    if (ndata < 2) {
        return;
    }

    size_t nleft  = ndata / 2;
    size_t nright = ndata - nleft;

    char **aux = LS_XNEW(char *, ndata);

    char **left  = aux;
    char **right = aux + nleft;

    copy(left,  data,         nleft);
    copy(right, data + nleft, nright);

    strset_mergesort(left,  nleft);
    strset_mergesort(right, nright);

    size_t i = 0;
    size_t j = 0;
    while (i < nleft && j < nright) {
        char *elem_left  = left[i];
        char *elem_right = right[j];
        if (strcmp(elem_left, elem_right) <= 0) {
            data[i + j] = elem_left;
            ++i;
        } else {
            data[i + j] = elem_right;
            ++j;
        }
    }

    while (i < nleft) {
        data[i + j] = left[i];
        ++i;
    }
    while (j < nright) {
        data[i + j] = right[j];
        ++j;
    }

    free(aux);
}
