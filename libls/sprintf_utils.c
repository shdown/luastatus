#include "sprintf_utils.h"

#include <assert.h>
#include <stdarg.h>

char*
ls_vasprintf(const char *fmt, va_list vl)
{
    char *ret = NULL;
    va_list vl2;
    va_copy(vl2, vl);
    int saved_errno;

    int r = vsnprintf(NULL, 0, fmt, vl);
    if (r < 0) {
        goto cleanup;
    }
    if (!(ret = malloc((size_t) r + 1))) {
        goto cleanup;
    }
    int r2 = vsnprintf(ret, (size_t) r + 1, fmt, vl2);
    (void) r2;
    assert(r2 == r);

cleanup:
    saved_errno = errno;
    va_end(vl2);
    errno = saved_errno;
    return ret;
}
