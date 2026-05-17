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

#include "strset_binsearch.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

// See file 'DEV_NOTES.txt' for why we roll our own sorting/binsearch
// implementation.

bool strset_binsearch(char **data, size_t ndata, const char *s)
{
    size_t l = 0;
    size_t r = ndata;
    // Invariants: l <= r, data at half-open interval [l; r) contains 's'.
    while (r - l > 1) {
        size_t m = l + (r - l) / 2;
        if (strcmp(data[m], s) <= 0) {
            l = m;
        } else {
            r = m;
        }
    }
    return l != r && strcmp(data[l], s) == 0;
}
