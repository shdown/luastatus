#ifndef ls_compdep_h_
#define ls_compdep_h_

#include <assert.h>
#include <stdlib.h> // for abort

#define LS_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#define LS_ATTR_UNUSED                          /*nothing*/
#define LS_ATTR_NORETURN                        /*nothing*/
#define LS_ATTR_PRINTF(FmtArgN_, EllipsisArgN_) /*nothing*/
#define LS_HAS_BUILTIN_UNREACHABLE              0

#if __GNUC__ >= 2
#   undef  LS_ATTR_UNUSED
#   define LS_ATTR_UNUSED __attribute__((unused))

#   undef  LS_ATTR_PRINTF
#   define LS_ATTR_PRINTF(FmtArgN_, EllipsisArgN_) \
        __attribute__((format(printf, FmtArgN_, EllipsisArgN_)))

#   undef  LS_ATTR_NORETURN
#   define LS_ATTR_NORETURN __attribute__((noreturn))
#endif

#if LS_GCC_VERSION >= 40500
#   undef  LS_HAS_BUILTIN_UNREACHABLE
#   define LS_HAS_BUILTIN_UNREACHABLE 1
#endif

#if defined(__has_builtin) /* clang */
#   undef  LS_HAS_BUILTIN_UNREACHABLE
#   define LS_HAS_BUILTIN_UNREACHABLE __has_builtin(__builtin_unreachable)
#endif

#if LS_HAS_BUILTIN_UNREACHABLE
#   define LS_UNREACHABLE() \
        do { \
            assert(0); \
            __builtin_unreachable(); \
        } while (0)
#else
#   define LS_UNREACHABLE() \
        do { \
            assert(0); \
            abort(); \
        } while (0)
#endif

#define LS_INHEADER static inline LS_ATTR_UNUSED

#endif
