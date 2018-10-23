#ifndef ls_compdep_h_
#define ls_compdep_h_

#include <stdlib.h> // for abort()

#define LS_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#if defined(__has_builtin)
#   define LS_CLANG_HAS_BUILTIN(X_) __has_builtin(X_)
#else
#   define LS_CLANG_HAS_BUILTIN(X_) 0
#endif

#if __GNUC__ >= 2
#   define LS_ATTR_UNUSED           __attribute__((unused))
#   define LS_ATTR_PRINTF(N_, M_)   __attribute__((format(printf, N_, M_)))
#   define LS_ATTR_NORETURN         __attribute__((noreturn))
#else
#   define LS_ATTR_UNUSED           /*nothing*/
#   define LS_ATTR_PRINTF           /*nothing*/
#   define LS_ATTR_NORETURN         /*nothing*/
#endif

#if LS_GCC_VERSION >= 40500 || LS_CLANG_HAS_BUILTIN(__builtin_unreachable)
#   define LS_UNREACHABLE() __builtin_unreachable()
#else
#   define LS_UNREACHABLE() abort()
#endif

#define LS_INHEADER static inline LS_ATTR_UNUSED

#endif
