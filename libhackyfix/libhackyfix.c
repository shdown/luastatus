/*
 * Copyright (C) 2025  luastatus developers
 *
 * This file is part of luastatus.
 *
 * luastatus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * luastatus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with luastatus.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include "get_rtld_next_handle.h"
#include "fatal.h"

// dlerror() is not thread-safe at least on NetBSD and musl.
// Let's assume other *BSDs and/or libcs have non-thread-safe dlerror() as well.

#if defined(__GLIBC__)
#   define REPLACE_DLERROR 0
#else
#   define REPLACE_DLERROR 1
#endif

static int (*orig_fflush)(FILE *f);
static struct lconv *(*orig_localeconv)(void);

static struct lconv my_lconv;

static char *xstrdup_or_null(const char *s)
{
    if (!s) {
        return NULL;
    }
    size_t ns = strlen(s);
    char *res = malloc(ns + 1);
    if (!res) {
        libhackyfix_fatal("FATAL: libhackyfix: malloc() failed\n");
    }
    memcpy(res, s, ns + 1);
    return res;
}

static void query_and_copy_lconv(void)
{
    struct lconv *cur = orig_localeconv();
    my_lconv = *cur;

#define copy(field) (my_lconv.field = xstrdup_or_null(cur->field))

    copy(decimal_point);
    copy(thousands_sep);
    copy(grouping);
    copy(int_curr_symbol);
    copy(currency_symbol);
    copy(mon_decimal_point);
    copy(mon_thousands_sep);
    copy(mon_grouping);
    copy(positive_sign);
    copy(negative_sign);

#undef copy
}

__attribute__((constructor))
static void initialize(void)
{
    void *rtld_next_handle = libhackyfix_get_rtld_next_handle();

    *(void **) &orig_fflush = dlsym(rtld_next_handle, "fflush");
    if (!orig_fflush) {
        libhackyfix_fatal("FATAL: libhackyfix: dlsym(..., \"fflush\") failed\n");
    }

    *(void **) &orig_localeconv = dlsym(rtld_next_handle, "localeconv");
    if (!orig_localeconv) {
        libhackyfix_fatal("FATAL: libhackyfix: dlsym(..., \"localeconv\") failed\n");
    }

    query_and_copy_lconv();
}

__attribute__((destructor))
static void deinitialize(void)
{
    free(my_lconv.decimal_point);
    free(my_lconv.thousands_sep);
    free(my_lconv.grouping);
    free(my_lconv.int_curr_symbol);
    free(my_lconv.currency_symbol);
    free(my_lconv.mon_decimal_point);
    free(my_lconv.mon_thousands_sep);
    free(my_lconv.mon_grouping);
    free(my_lconv.positive_sign);
    free(my_lconv.negative_sign);
}

// ==================================================
// Replacement for /fflush/
// ==================================================

int fflush(FILE *f)
{
    if (!f) {
        return 0;
    }
    return orig_fflush(f);
}

// ==================================================
// Replacement for /strerror/
// ==================================================

#if _POSIX_C_SOURCE < 200112L || defined(_GNU_SOURCE)
#   error "Unsupported feature test macros."
#endif

static __thread char errbuf[512];

char *strerror(int errnum)
{
    // We introduce an /int/ variable in order to get a compilation warning if /strerror_r()/ is
    // still GNU-specific and returns a pointer to char.
    int r = strerror_r(errnum, errbuf, sizeof(errbuf));
    return r == 0 ? errbuf : (char *) "unknown error or truncated error message";
}

// ==================================================
// Replacement for /getenv/
// ==================================================

extern char **environ;

char *getenv(const char *name)
{
    if (!environ) {
        return NULL;
    }
    if ((strchr(name, '='))) {
        return NULL;
    }
    size_t nname = strlen(name);
    for (char **s = environ; *s; ++s) {
        const char *entry = *s;
        if (strncmp(entry, name, nname) == 0 && entry[nname] == '=') {
            return (char *) (entry + nname + 1);
        }
    }
    return NULL;
}

// ==================================================
// Replacement for /localeconv/
// ==================================================
struct lconv *localeconv(void)
{
    return &my_lconv;
}

#if REPLACE_DLERROR

// ==================================================
// Replacement for /dlerror/
// ==================================================

char *dlerror(void)
{
    return (char *) "(no error message for you because on your system dlerror() isn't thread-safe)";
}

#endif
