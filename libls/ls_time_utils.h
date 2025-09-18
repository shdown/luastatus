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
#include <limits.h>
#include <assert.h>
#include <unistd.h>

#include "ls_compdep.h"
#include "ls_panic.h"

// Time deltas >= LS_TMO_MAX (in seconds) are treated as "forever".
#define LS_TMO_MAX 2147483647.0

// These are wrapped into structs so that it's hard to do the wrong thing.

// A time stamp. Can be either valid or "bad".
typedef struct {
    double ts;
} LS_TimeStamp;

// A time delta. Can be either valid or "forever".
typedef struct {
    double delta;
} LS_TimeDelta;

#define LS_TD_FOREVER ((LS_TimeDelta) {LS_TMO_MAX})

#define LS_TD_ZERO ((LS_TimeDelta) {0.0})

#define LS_TS_BAD ((LS_TimeStamp) {-1.0})

LS_INHEADER bool ls_double_is_good_time_delta(double x)
{
    return x >= 0;
}

LS_INHEADER bool ls_TD_is_forever(LS_TimeDelta TD)
{
    return TD.delta >= LS_TMO_MAX;
}

LS_INHEADER bool ls_TS_is_bad(LS_TimeStamp TS)
{
    return TS.ts < 0;
}

LS_INHEADER bool ls_double_to_TD_checked(double delta, LS_TimeDelta *out)
{
    if (!(delta >= 0)) {
        return false;
    }
    if (delta > LS_TMO_MAX) {
        delta = LS_TMO_MAX;
    }
    *out = (LS_TimeDelta) {delta};
    return true;
}

LS_INHEADER LS_TimeDelta ls_double_to_TD(double delta, LS_TimeDelta if_bad)
{
    LS_TimeDelta res;
    if (ls_double_to_TD_checked(delta, &res)) {
        return res;
    }
    return if_bad;
}

LS_INHEADER LS_TimeDelta ls_double_to_TD_or_die(double delta)
{
    LS_TimeDelta res;
    if (!ls_double_to_TD_checked(delta, &res)) {
        LS_PANIC("ls_double_to_TD_or_die() failed");
    }
    return res;
}

LS_INHEADER double ls_timespec_to_raw_double_(struct timespec ts)
{
    double s = ts.tv_sec;
    double ns = ts.tv_nsec;
    return s + ns / 1e9;
}

LS_INHEADER LS_TimeStamp ls_timespec_to_TS(struct timespec ts)
{
    return (LS_TimeStamp) {ls_timespec_to_raw_double_(ts)};
}

LS_INHEADER LS_TimeDelta ls_timespec_to_TD(struct timespec ts)
{
    return (LS_TimeDelta) {ls_timespec_to_raw_double_(ts)};
}

LS_INHEADER struct timespec ls_now_timespec(void)
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

LS_INHEADER LS_TimeStamp ls_now(void)
{
    return ls_timespec_to_TS(ls_now_timespec());
}

LS_INHEADER struct timespec ls_TD_to_timespec(LS_TimeDelta TD)
{
    assert(!ls_TD_is_forever(TD));

    double delta = TD.delta;
    return (struct timespec) {
        .tv_sec = delta,
        .tv_nsec = (delta - (time_t) delta) * 1e9,
    };
}

LS_INHEADER struct timeval ls_TD_to_timeval(LS_TimeDelta TD)
{
    assert(!ls_TD_is_forever(TD));

    double delta = TD.delta;
    return (struct timeval) {
        .tv_sec = delta,
        .tv_usec = (delta - (time_t) delta) * 1e6,
    };
}

LS_INHEADER int ls_TD_to_poll_ms_tmo(LS_TimeDelta TD)
{
    if (ls_TD_is_forever(TD)) {
        return -1;
    }
    double ms = TD.delta * 1000;
    return ms > INT_MAX ? INT_MAX : ms;
}

LS_INHEADER LS_TimeDelta ls_TS_minus_TS_nonneg(LS_TimeStamp a, LS_TimeStamp b)
{
    assert(!ls_TS_is_bad(a));
    assert(!ls_TS_is_bad(b));

    double res = a.ts - b.ts;
    return (LS_TimeDelta) {res < 0 ? 0 : res};
}

LS_INHEADER LS_TimeStamp ls_TS_plus_TD(LS_TimeStamp a, LS_TimeDelta b)
{
    assert(!ls_TS_is_bad(a));
    assert(!ls_TD_is_forever(b));

    return (LS_TimeStamp) {a.ts + b.delta};
}

LS_INHEADER bool ls_TD_less(LS_TimeDelta a, LS_TimeDelta b)
{
    if (ls_TD_is_forever(a) && ls_TD_is_forever(b)) {
        return false;
    }
    return a.delta < b.delta;
}

LS_INHEADER void ls_sleep(LS_TimeDelta TD)
{
    if (ls_TD_is_forever(TD)) {
        for (;;) {
            pause();
        }
    }

    struct timespec ts = ls_TD_to_timespec(TD);
    struct timespec rem;
    while (nanosleep(&ts, &rem) < 0 && errno == EINTR) {
        ts = rem;
    }
}

LS_INHEADER void ls_sleep_simple(double seconds)
{
    ls_sleep(ls_double_to_TD_or_die(seconds));
}

#endif
