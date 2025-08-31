/*
 * Copyright (C) 2015-2020  luastatus developers
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

#ifndef ls_prng_h_
#define ls_prng_h_

#include <stdint.h>
#include "libls/compdep.h"

// MINSTD pseudo-random number generator, year 1990 version.

uint32_t ls_prng_make_up_some_seed(void);

typedef struct {
    uint32_t x;
} LS_Prng;

enum {
    LS_PRNG_M = 0x7fffffff,
    LS_PRNG_A = 48271,
};

LS_INHEADER LS_Prng ls_prng_new(uint32_t seed)
{
    uint32_t res = seed % LS_PRNG_M;
    if (!res) {
        res = 1 + (seed % (LS_PRNG_M - 1));
    }
    return (LS_Prng) {res};
}

LS_INHEADER uint32_t ls_prng_next_u32(LS_Prng *P)
{
    uint64_t prod = ((uint64_t) P->x) * LS_PRNG_A;
    P->x = prod % LS_PRNG_M;
    return P->x;
}

LS_INHEADER uint64_t ls_prng_next_u64(LS_Prng *P)
{
    uint32_t lo = ls_prng_next_u32(P);
    uint32_t hi = ls_prng_next_u32(P);
    return lo | (((uint64_t) hi) << 32);
}

// Returns random integer x such that 0 <= x < limit.
// If limit == 0, the behavior is undefined.
LS_INHEADER uint32_t ls_prng_next_limit_u32(LS_Prng *P, uint32_t limit)
{
    // /reject_thres/ is the largest value that fits into /uint64_t/ and
    // is divisible by /limit/.
    uint64_t reject_thres = UINT64_MAX - UINT64_MAX % limit;

    uint64_t v;
    do {
        v = ls_prng_next_u64(P);
    } while (v >= reject_thres);
    return v % limit;
}

#endif
