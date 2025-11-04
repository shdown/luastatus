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

__attribute__((constructor))
static void initialize(void)
{
    void *rtld_next_handle = libhackyfix_get_rtld_next_handle();
    *(void **) &orig_fflush = dlsym(rtld_next_handle, "fflush");
    if (!orig_fflush) {
        libhackyfix_fatal("FATAL: libhackyfix: dlsym(..., \"fflush\") failed\n");
    }
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

#if REPLACE_DLERROR

// ==================================================
// Replacement for /dlerror/
// ==================================================

char *dlerror(void)
{
    return (char *) "(no error message for you because on your system dlerror() isn't thread-safe)";
}

#endif
