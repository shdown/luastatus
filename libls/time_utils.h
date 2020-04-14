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

#ifndef ls_time_utils_h_
#define ls_time_utils_h_

#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <stdbool.h>

#include "compdep.h"

static const LS_ATTR_UNUSED struct timespec ls_timespec_invalid = {.tv_nsec = -1};
static const LS_ATTR_UNUSED struct timeval  ls_timeval_invalid  = {.tv_usec = -1};

LS_INHEADER
bool
ls_timespec_is_invalid(struct timespec ts)
{
    return ts.tv_nsec == -1;
}

LS_INHEADER
bool
ls_timeval_is_invalid(struct timeval tv)
{
    return tv.tv_usec == -1;
}

// Converts the number of seconds specified by /seconds/ to a /struct timespec/.
// Returns /ls_timespec_invalid/ if /seconds/ is negative, NaN, or too big.
struct timespec
ls_timespec_from_seconds(double seconds);

// Converts the number of seconds specified by /seconds/ to a /struct timeval/.
// Returns /ls_timeval_invalid/ if /seconds/ is negative, NaN, or too big.
struct timeval
ls_timeval_from_seconds(double seconds);

// Converts the number of seconds specified by /seconds/ to a /struct timespec/, writing the result
// or /ls_timespec_invalid/ to /*out/.
//
// Returns /true/ if either:
//   * /seconds/ has an explicitly "invalid" value (negative or NaN), or;
//   * the conversion was successful.
//
// Returns /false/ on overflow.
LS_INHEADER
bool
ls_opt_timespec_from_seconds(double seconds, struct timespec *out)
{
    *out = ls_timespec_from_seconds(seconds);
    return !(ls_timespec_is_invalid(*out) && seconds >= 0);
}

// Converts the number of seconds specified by /seconds/ to a /struct timeval/, writing the result
// or /ls_timeval_invalid/ to /*out/.
//
// Returns /true/ if either:
//   * /seconds/ has an explicitly "invalid" value (negative or NaN), or;
//   * the conversion was successful.
//
// Returns /false/ on overflow.
LS_INHEADER
bool
ls_opt_timeval_from_seconds(double seconds, struct timeval *out)
{
    *out = ls_timeval_from_seconds(seconds);
    return !(ls_timeval_is_invalid(*out) && seconds >= 0);
}

LS_INHEADER
void
ls_nanosleep(struct timespec ts)
{
    struct timespec rem;
    while (nanosleep(&ts, &rem) < 0 && errno == EINTR) {
        ts = rem;
    }
}

#endif
