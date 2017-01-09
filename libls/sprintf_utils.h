#ifndef ls_sprintf_utils_h_
#define ls_sprintf_utils_h_

#include <errno.h>
#include <stdarg.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "compdep.h"
#include "cstring_utils.h"
#include "errno_utils.h"

LS_INHEADER
void
ls_svsnprintf(char *buf, size_t nbuf, const char *fmt, va_list vl)
{
    if (vsnprintf(buf, nbuf, fmt, vl) < 0) {
        ls_strlcpy(buf, "(vsnprintf failed)", nbuf);
    }
}

LS_INHEADER LS_ATTR_PRINTF(3, 4)
void
ls_ssnprintf(char *buf, size_t nbuf, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    ls_svsnprintf(buf, nbuf, fmt, vl);
    va_end(vl);
}

LS_INHEADER
void
ls_xvsnprintf(char *buf, size_t nbuf, const char *fmt, va_list vl)
{
    int r = vsnprintf(buf, nbuf, fmt, vl);
    if (r < 0) {
        LS_WITH_ERRSTR(s, errno,
            fprintf(stderr, "ls_xvsnprintf: %s\n", s);
        );
        abort();
    } else if ((size_t) r >= nbuf) {
        fputs("ls_xvsnprintf: insufficient buffer space\n", stderr);
        abort();
    }
}

LS_INHEADER LS_ATTR_PRINTF(3, 4)
void
ls_xsnprintf(char *buf, size_t nbuf, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    ls_xvsnprintf(buf, nbuf, fmt, vl);
    va_end(vl);
}

char*
ls_vasprintf(const char *fmt, va_list vl);

LS_INHEADER LS_ATTR_PRINTF(1, 2)
char*
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

LS_INHEADER
char*
ls_xvasprintf(const char *fmt, va_list vl)
{
    char *r = ls_vasprintf(fmt, vl);
    if (!r) {
        LS_WITH_ERRSTR(s, errno,
            fprintf(stderr, "ls_xvasprintf: %s\n", s);
        );
        abort();
    }
    return r;
}

LS_INHEADER LS_ATTR_PRINTF(1, 2)
char*
ls_xasprintf(const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    char *r = ls_xvasprintf(fmt, vl);
    va_end(vl);
    return r;
}

#endif
