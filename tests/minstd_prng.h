/*
 * Copyright (C) 2021  luastatus developers
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

#ifndef luastatus_tests_minstd_prng_h_
#define luastatus_tests_minstd_prng_h_

#include <stdint.h>
#include "libls/compdep.h"

// MINSTD pseudo-random number generator, year 1990 version.

typedef struct {
    uint32_t x;
} MinstdPRNG;

enum {
    MINSTD_PRNG_M = 0x7fffffff,
    MINSTD_PRNG_A = 48271,
};

LS_INHEADER MinstdPRNG minstd_prng_new(uint32_t seed)
{
    uint32_t res = seed % MINSTD_PRNG_M;
    if (!res) {
        res = 1 + (seed % (MINSTD_PRNG_M - 1));
    }
    return (MinstdPRNG) {res};
}

LS_INHEADER uint32_t minstd_prng_next_u32(MinstdPRNG *P)
{
    uint64_t prod = ((uint64_t) P->x) * MINSTD_PRNG_A;
    P->x = prod % MINSTD_PRNG_M;
    return P->x;
}

LS_INHEADER uint64_t minstd_prng_next_u64(MinstdPRNG *P)
{
    uint32_t lo = minstd_prng_next_u32(P);
    uint32_t hi = minstd_prng_next_u32(P);
    return lo | (((uint64_t) hi) << 32);
}

#endif
