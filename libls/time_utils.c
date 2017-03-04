#include "time_utils.h"

#include <time.h>
#include <sys/time.h>
#include <errno.h>

// time_t can't be less that 31 bits, can it?...
static const double TIME_T_LIMIT = 2147483647.;

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
