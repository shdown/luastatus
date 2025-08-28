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
#include <limits.h>

#include "compdep.h"
#include "panic.h"

#define LS_TMO_MAX 2147483647.0

LS_INHEADER double ls_ts_to_tmo(struct timespec ts)
{
    double s = ts.tv_sec;
    double ns = ts.tv_nsec;
    return s + ns / 1e9;
}

LS_INHEADER struct timespec ls_now_ts(void)
{
    struct timespec ts;
    int rc;

#if (!defined(_POSIX_MONOTONIC_CLOCK)) || (_POSIX_MONOTONIC_CLOCK < 0)
    // CLOCK_MONOTONIC is not supported at compile-time.
    rc = clock_gettime(CLOCK_REALTIME, &ts);

#elif _POSIX_MONOTONIC_CLOCK > 0
    // CLOCK_MONOTONIC is supported both at compile-time and at run-time.
    rc = clock_gettime(CLOCK_MONOTONIC, &ts);

#else
    // CLOCK_MONOTONIC is supported at compile-time, but might or might not
    // be supported at run-time.
    rc = clock_gettime(CLOCK_MONOTONIC, &ts);
    if (rc < 0) {
        rc = clock_gettime(CLOCK_REALTIME, &ts);
    }
#endif

    if (rc < 0) {
        LS_PANIC("clock_gettime() failed");
    }
    return ts;
}

LS_INHEADER double ls_now(void)
{
    return ls_ts_to_tmo(ls_now_ts());
}

LS_INHEADER struct timespec ls_tmo_to_ts(double tmo)
{
    if (!(tmo >= 0))
        tmo = 0;
    if (tmo > LS_TMO_MAX)
        tmo = LS_TMO_MAX;
    return (struct timespec) {
        .tv_sec = tmo,
        .tv_nsec = (tmo - (time_t) tmo) * 1e9,
    };
}

LS_INHEADER struct timeval ls_tmo_to_tv(double tmo)
{
    if (!(tmo >= 0))
        tmo = 0;
    if (tmo > LS_TMO_MAX)
        tmo = LS_TMO_MAX;
    return (struct timeval) {
        .tv_sec = tmo,
        .tv_usec = (tmo - (time_t) tmo) * 1e6,
    };
}

LS_INHEADER int ls_tmo_to_ms(double tmo)
{
    if (tmo != tmo)
        return 0;
    if (tmo < 0.0)
        return -1;
    double ms = tmo * 1000;
    return ms > INT_MAX ? INT_MAX : ms;
}

LS_INHEADER void ls_sleep(double tmo)
{
    struct timespec ts = ls_tmo_to_ts(tmo);
    struct timespec rem;
    while (nanosleep(&ts, &rem) < 0 && errno == EINTR) {
        ts = rem;
    }
}

#endif
