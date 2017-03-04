#ifndef ls_time_utils_h_
#define ls_time_utils_h_

#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
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

struct timespec
ls_timespec_from_seconds(double seconds);

struct timeval
ls_timeval_from_seconds(double seconds);

#endif
