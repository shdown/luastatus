#ifndef ls_time_utils_h_
#define ls_time_utils_h_

#include <time.h>
#include <sys/time.h>
#include <stdbool.h>

#include "compdep.h"

// Special "invalid" values for /struct timespec/ and /struct timeval/.
static const struct timespec ls_timespec_invalid = {.tv_nsec = -1};
static const struct timeval  ls_timeval_invalid  = {.tv_usec = -1};

// Checks if the value of /ts/ is "invalid".
LS_INHEADER
bool
ls_timespec_is_invalid(struct timespec ts)
{
    return ts.tv_nsec == -1;
}

// Checks if the value of /tv/ is "invalid".
LS_INHEADER
bool
ls_timeval_is_invalid(struct timeval tv)
{
    return tv.tv_usec == -1;
}

// Converts the number of seconds specified by /seconds/ to a /struct timespec/.
// Returns an "invalid" value if /seconds/ is either negative or too big.
struct timespec
ls_timespec_from_seconds(double seconds);

// Converts the number of seconds specified by /seconds/ to a /struct timeval/.
// Returns an "invalid" value if /seconds/ is either negative or too big.
struct timeval
ls_timeval_from_seconds(double seconds);

#endif
