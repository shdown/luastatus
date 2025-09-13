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

#ifndef httpserv_fatal_error_h_
#define httpserv_fatal_error_h_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define FATAL(...) \
    do { \
        fprintf(stderr, "FATAL: httpserv: " __VA_ARGS__); \
        abort(); \
    } while (0)

#define PERROR_AND_DIE(S_) \
    FATAL("%s: %s\n", (S_), strerror(errno))

#define PERROR2_AND_DIE(S1_, S2_) \
    FATAL("%s: %s: %s\n", (S1_), (S2_), strerror(errno))

#define OUT_OF_MEMORY() FATAL("out of memory.\n")

#endif
