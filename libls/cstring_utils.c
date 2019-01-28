#include "cstring_utils.h"

#include <string.h>

const char *
ls_strerror_r(int errnum, char *buf, size_t nbuf)
{
#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! defined(_GNU_SOURCE)
    // XSI-compliant /strerror_r/
    return strerror_r(errnum, buf, nbuf) == 0 ? buf : "unknown error";
#else
    // GNU-specific /strerror_r/
    return strerror_r(errnum, buf, nbuf);
#endif
}
