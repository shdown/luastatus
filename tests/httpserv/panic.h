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

#ifndef httpserv_panic_h_
#define httpserv_panic_h_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// We are single-threaded, so a simple strerror() is OK.
#define my_strerror strerror

#define FATAL(...) \
    do { \
        fprintf(stderr, "httpserv: " __VA_ARGS__); \
        abort(); \
    } while (0)

#define OUT_OF_MEMORY() FATAL("out of memory.\n")

#define MY_ASSERT(Expr_) \
    do { \
        if (!(Expr_)) { \
            FATAL( \
                "MY_ASSERT(%s) failed in %s at %s:%d\n", \
                #Expr_, __func__, __FILE__, __LINE__); \
        } \
    } while (0)

#define MY_UNREACHABLE() \
    FATAL( \
        "MY_UNREACHABLE() in %s at %s:%d\n", \
        __func__, __FILE__, __LINE__)

#endif
