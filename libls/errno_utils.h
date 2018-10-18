#ifndef ls_errno_utils_h_
#define ls_errno_utils_h_

#include <string.h>

#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! defined(_GNU_SOURCE)
    // XSI-compliant /strerror_r/
#   define LS_WITH_ERRSTR(Var_, Errnum_, ...) \
    do { \
        char Var_[256]; \
        if (strerror_r(Errnum_, Var_, sizeof(Var_)) != 0) { \
            Var_[0] = '\0'; \
        } \
        __VA_ARGS__ \
    } while (0)
#else
    // GNU-specific /strerror_r/
#   define LS_WITH_ERRSTR(Var_, Errnum_, ...) \
    do { \
        char strerrbuf__[256]; \
        const char *Var_ = strerror_r(Errnum_, strerrbuf__, sizeof(strerrbuf__)); \
        __VA_ARGS__ \
    } while (0)
#endif

#endif
