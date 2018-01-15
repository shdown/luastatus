#ifndef ls_sprintf_utils_h_
#define ls_sprintf_utils_h_

#include <errno.h>
#include <stdarg.h>

#include "alloc_utils.h"
#include "compdep.h"

// Tries to allocate (as if with /malloc/) a buffer of a sufficient size and
// /vsnprintf(<unspecified>, fmt, vl)/ into it. On success, returns the buffer.
//
// If an encoding error occurs, /NULL/ is returned and /errno/ is set by /vsnprintf/.
//
// If allocation fails, /NULL/ is returned and /errno/ is set to /ENOMEM/.
char *
ls_vasprintf(const char *fmt, va_list vl);

// The same as /ls_vasprintf/, except that this function is called with a variable number of
// arguments instead of a /va_list/.
LS_INHEADER LS_ATTR_PRINTF(1, 2)
char *
ls_asprintf(const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    char *r = ls_vasprintf(fmt, vl);
    int saved_errno = errno;
    va_end(vl);
    errno = saved_errno;
    return r;
}

// The behaviour is same as calling /ls_vasprintf(fmt, vl)/, except when allocation fails. In that
// case, this function panics.
LS_INHEADER
char *
ls_xvasprintf(const char *fmt, va_list vl)
{
    char *r = ls_vasprintf(fmt, vl);
    if (!r && errno == ENOMEM) {
        ls_oom();
    }
    return r;
}

// The behaviour is same as calling /ls_asprintf(fmt, ...)/, except when allocation fails. In that
// case, this function panics.
LS_INHEADER LS_ATTR_PRINTF(1, 2)
char *
ls_xasprintf(const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    char *r = ls_xvasprintf(fmt, vl);
    int saved_errno = errno;
    va_end(vl);
    errno = saved_errno;
    return r;
}

#endif
