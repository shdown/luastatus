/*
 * Copyright (C) 2026  luastatus developers
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

#pragma once

#include <stddef.h>
#include <stdarg.h>

#if __GNUC__ >= 2
#   define ATTR_PRINTF(i, j)   __attribute__((format(printf, (i), (j))))
#   define ATTR_NORETURN       __attribute__((noreturn))
#else
#   define ATTR_PRINTF(i, j)   /*nothing*/
#   define ATTR_NORETURN       /*nothing*/
#endif

void *xmalloc(size_t n, size_t m);

void *xrealloc(void *p, size_t n, size_t m);

void *x2realloc(void *p, size_t *pcapacity, size_t elemsz);

char *xstrdup(const char *s);

void *xmemdup(const void *p, size_t n);

ATTR_PRINTF(1, 2)
char *xallocf(const char *fmt, ...);

ATTR_PRINTF(1, 0)
char *xallocvf(const char *fmt, va_list vl);

ATTR_NORETURN ATTR_PRINTF(1, 2)
void xpanicf(const char *fmt, ...);

ATTR_NORETURN ATTR_PRINTF(1, 0)
void xpanicvf(const char *fmt, va_list vl);

#define xpanic_oom() xpanicf("out of memory.\n")

#define xassert(expr) \
    do { \
        if (!(expr)) { \
            xpanicf( \
                "xassert(%s) failed in %s at %s:%d.\n", \
                #expr, \
                __func__, \
                __FILE__, \
                __LINE__ \
            ); \
        } \
    } while (0)
