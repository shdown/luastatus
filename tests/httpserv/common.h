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

#include <stdlib.h>
#include <stdio.h>

#define xmalloc(n, m) xrealloc(NULL, (n), (m))

void *xrealloc(void *p, size_t n, size_t m);

void *x2realloc(void *p, size_t *pcapacity, size_t elem_sz);

void *xmemdup(const void *p, size_t n);

char *xstrdup(const char *s);

#define panic(s) (fprintf(stderr, "FATAL: %s\n", (s)), abort())

#define panic_oom() panic("out of memory")

#if __GNUC__ >= 2
#   define ATTR_NORETURN __attribute__((noreturn))
#else
#   define ATTR_NORETURN /*nothing*/
#endif
