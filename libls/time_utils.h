#ifndef ls_time_utils_h_
#define ls_time_utils_h_

#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <errno.h>
#include "compdep.h"

static const struct timespec ls_timespec_invalid = {0, -1};
static const struct timeval  ls_timeval_invalid = {0, -1};

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

static const double TIME_T_LIMIT = 2147483647.;

LS_INHEADER
struct timespec
ls_timespec_from_seconds(double seconds)
{
    if (seconds < 0) {
        errno = EDOM;
        return ls_timespec_invalid;
    }
    if (seconds > TIME_T_LIMIT) {
        errno = ERANGE;
        return ls_timespec_invalid;
    }
    return (struct timespec) {
        .tv_sec = seconds,
        .tv_nsec = (seconds - (time_t) seconds) * 1e9,
    };
}

LS_INHEADER
struct timeval
ls_timeval_from_seconds(double seconds)
{
    if (seconds < 0) {
        errno = EDOM;
        return ls_timeval_invalid;
    }
    if (seconds > TIME_T_LIMIT) {
        errno = ERANGE;
        return ls_timeval_invalid;
    }
    return (struct timeval) {
        .tv_sec = seconds,
        .tv_usec = (seconds - (time_t) seconds) * 1e6,
    };
}

#endif
