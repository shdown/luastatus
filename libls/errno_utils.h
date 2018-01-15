#ifndef ls_errno_utils_h_
#define ls_errno_utils_h_

#include <stddef.h>
#include <string.h>

#include "cstring_utils.h"
#include "compdep.h"

// Like /strerror_r/, but always fills /buf/ with something (and is GNU-safe).
// May alter /errno/.
LS_INHEADER
void
ls_strerror_r(int errnum, char *buf, size_t nbuf)
{
#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! defined(_GNU_SOURCE)
    // XSI-compliant /strerror_r/
    if (strerror_r(errnum, buf, nbuf) != 0) {
        ls_strlcpy(buf, "Unknown error", nbuf);
    }
#else
    // GNU-specific /strerror_r/
    const char *s = strerror_r(errnum, buf, nbuf);
    if (s != buf) {
        ls_strlcpy(buf, s, nbuf);
    }
#endif
}

// Defines a /char/ array variable named /BufVar_/ in a new block, fills it with description of
// error with number /ErrNum_/, and then expands /__VA_ARGS__/.
//
// May alter /errno/ before or after expanding /__VA_ARGS__/.
#define LS_WITH_ERRSTR(BufVar_, ErrNum_, ...) \
    do { \
        char BufVar_[256]; \
        ls_strerror_r(ErrNum_, BufVar_, sizeof(BufVar_)); \
        __VA_ARGS__ \
    } while (0)

#endif
