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

#ifndef minstd_h_
#define minstd_h_

#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include "libls/compdep.h"

// MINSTD pseudo-random number generator, year 1990 version.

LS_INHEADER uint32_t minstd_make_up_some_seed(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) < 0) {
        return getpid();
    }

    uint32_t nsec = ts.tv_nsec;
    uint32_t sec = ts.tv_sec;

    return nsec + sec * (1000 * 1000 * 1000);
}

typedef struct {
    uint32_t x;
} MINSTD_Prng;

enum {
    MINSTD_M = 0x7fffffff,
    MINSTD_A = 48271,
};

LS_INHEADER MINSTD_Prng minstd_prng_new(uint32_t seed)
{
    uint32_t res = seed % MINSTD_M;
    if (!res) {
        res = 1 + (seed % (MINSTD_M - 1));
    }
    return (MINSTD_Prng) {res};
}

LS_INHEADER uint32_t minstd_prng_next_u32(MINSTD_Prng *P)
{
    uint64_t prod = ((uint64_t) P->x) * MINSTD_A;
    P->x = prod % MINSTD_M;
    return P->x;
}

LS_INHEADER uint64_t minstd_prng_next_u64(MINSTD_Prng *P)
{
    uint32_t lo = minstd_prng_next_u32(P);
    uint32_t hi = minstd_prng_next_u32(P);
    return lo | (((uint64_t) hi) << 32);
}

// Returns random integer x such that 0 <= x < limit.
// If limit == 0, the behavior is undefined.
LS_INHEADER uint32_t minstd_prng_next_limit_u32(MINSTD_Prng *P, uint32_t limit)
{
    // /reject_thres/ is the largest value that fits into /uint64_t/ and
    // is divisible by /limit/.
    uint64_t reject_thres = UINT64_MAX - UINT64_MAX % limit;

    uint64_t v;
    do {
        v = minstd_prng_next_u64(P);
    } while (v >= reject_thres);
    return v % limit;
}

#endif
