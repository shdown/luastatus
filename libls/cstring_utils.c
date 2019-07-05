#if _POSIX_C_SOURCE < 200112L || defined(_GNU_SOURCE)
#   error "Unsupported feature test macros; either tune them or change the code."
#endif

#include "cstring_utils.h"

#include <string.h>

const char *
ls_strerror_r(int errnum, char *buf, size_t nbuf)
{
    // We introduce an /int/ variable in order to get a compilation warning if /strerror_r()/ is
    // still GNU-specific and returns a pointer to char.
    const int r = strerror_r(errnum, buf, nbuf);
    return r == 0 ? buf : "unknown error or truncated error message";
}
