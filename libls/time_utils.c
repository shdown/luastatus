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

#include "time_utils.h"

#include <time.h>
#include <sys/time.h>

#include "algo.h"

// /time_t/ can't be less than 31 bits, can it?...
static const double TIME_T_LIMIT = 2147483647.;

struct timespec
ls_timespec_from_seconds(double seconds)
{
    if (!ls_is_between_d(seconds, 0, TIME_T_LIMIT)) {
        return ls_timespec_invalid;
    }
    return (struct timespec) {
        .tv_sec = seconds,
        .tv_nsec = (seconds - (time_t) seconds) * 1e9,
    };
}

struct timeval
ls_timeval_from_seconds(double seconds)
{
    if (!ls_is_between_d(seconds, 0, TIME_T_LIMIT)) {
        return ls_timeval_invalid;
    }
    return (struct timeval) {
        .tv_sec = seconds,
        .tv_usec = (seconds - (time_t) seconds) * 1e6,
    };
}
