/*
 * Copyright (C) 2015-2020  luastatus developers
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

#ifndef ls_vector_h_
#define ls_vector_h_

#include <stddef.h>

#include "alloc_utils.h"

// To be able to pass vectors as function arguments and/or return them, use, e.g.,
//
// typedef LS_VECTOR_OF(int) IntVector;
//
// Note this is not required for any other use.

#define LS_VECTOR_OF(Type_) \
    struct { \
        Type_ *data; \
        size_t size, capacity; \
    }

#define LS_VECTOR_NEW() \
    {NULL, 0, 0}

#define LS_VECTOR_NEW_RESERVE(Type_, Capacity_) \
    {LS_XNEW(Type_, Capacity_), 0, Capacity_}

#define LS_VECTOR_INIT(Vec_) \
    do { \
        (Vec_).data = NULL; \
        (Vec_).size = 0; \
        (Vec_).capacity = 0; \
    } while (0)

#define LS_VECTOR_INIT_RESERVE(Vec_, Capacity_) \
    do { \
        (Vec_).data = ls_xmalloc(Capacity_, sizeof(*(Vec_).data)); \
        (Vec_).size = 0; \
        (Vec_).capacity = (Capacity_); \
    } while (0)

#define LS_VECTOR_CLEAR(Vec_) \
    do { \
        (Vec_).size = 0; \
    } while (0)

#define LS_VECTOR_RESERVE(Vec_, ReqCapacity_) \
    do { \
        if ((Vec_).capacity < (ReqCapacity_)) { \
            (Vec_).capacity = (ReqCapacity_); \
            (Vec_).data = ls_xrealloc((Vec_).data, (Vec_).capacity, sizeof(*(Vec_).data)); \
        } \
    } while (0)

#define LS_VECTOR_ENSURE(Vec_, ReqCapacity_) \
    do { \
        while ((Vec_).capacity < (ReqCapacity_)) { \
            (Vec_).data = ls_x2realloc((Vec_).data, &(Vec_).capacity, sizeof(*(Vec_).data)); \
        } \
    } while (0)

#define LS_VECTOR_SHRINK(Vec_) \
    do { \
        (Vec_).capacity = (Vec_).size; \
        (Vec_).data = ls_xrealloc((Vec_).data, (Vec_).capacity, sizeof(*(Vec_).data)); \
    } while (0)

#define LS_VECTOR_PUSH(Vec_, Elem_) \
    do { \
        if ((Vec_).capacity == (Vec_).size) { \
            (Vec_).data = ls_x2realloc((Vec_).data, &(Vec_).capacity, sizeof(*(Vec_).data)); \
        } \
        (Vec_).data[(Vec_).size++] = (Elem_); \
    } while (0)

#define LS_VECTOR_FREE(Vec_) free((Vec_).data)

#endif
