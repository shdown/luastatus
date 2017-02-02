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

// The behaviour is same as calling vsnprintf(buf, nbuf, fmt, vl), except when an encoding error
// occurs. In that case, this function fills the buffer with an error message (unless nbuf is zero).
LS_INHEADER
void
ls_svsnprintf(char *buf, size_t nbuf, const char *fmt, va_list vl)
{
    if (vsnprintf(buf, nbuf, fmt, vl) < 0) {
        ls_strlcpy(buf, "(vsnprintf failed)", nbuf);
    }
}

// The behaviour is same as calling snprintf(buf, nbuf, fmt, ...), except when an encoding error
// occurs. In that case, this function fills the buffer with an error message (unless nbuf is zero).
LS_INHEADER LS_ATTR_PRINTF(3, 4)
void
ls_ssnprintf(char *buf, size_t nbuf, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    ls_svsnprintf(buf, nbuf, fmt, vl);
    va_end(vl);
}

// The behaviour is same as calling vsnprintf(buf, nbuf, fmt, vl), except when:
//     1. an encoding error occurs, or;
//     2. buffer space is insufficient.
//  In these cases, this function panics.
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

// The behaviour is same as calling snprintf(buf, nbuf, fmt, ...), except when:
//     1. an encoding error occurs, or;
//     2. buffer space is insufficient.
//  In these cases, this function panics.
LS_INHEADER LS_ATTR_PRINTF(3, 4)
void
ls_xsnprintf(char *buf, size_t nbuf, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    ls_xvsnprintf(buf, nbuf, fmt, vl);
    va_end(vl);
}

// Tries to allocate (with malloc, calloc or realloc) a buffer of sufficient size and
// vsnprintf([...], fmt, vl) into it. On success, returns the buffer.
//
// If an encoding error occurs, NULL is returned and errno is set by vsnprintf.
//
// If allocation fails, NULL is returned and errno is set to ENOMEM.
char *
ls_vasprintf(const char *fmt, va_list vl);

// The same as ls_vasprintf, except that this function is called with a variable number of arguments
// instead of a va_list.
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

// The behaviour is same as calling ls_vasprintf(fmt, vl), except when the latter returns NULL. In
// that case, this function panics.
LS_INHEADER
char *
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

// The behaviour is same as calling ls_asprintf(fmt, ...), except when the latter returns NULL. In
// that case, this function panics.
LS_INHEADER LS_ATTR_PRINTF(1, 2)
char *
ls_xasprintf(const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    char *r = ls_xvasprintf(fmt, vl);
    va_end(vl);
    return r;
}

#endif
