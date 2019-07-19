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
