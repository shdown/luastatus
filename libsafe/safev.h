/*
 * Copyright (C) 2015-2025  luastatus developers
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

#ifndef libsafe_safev_h_
#define libsafe_safev_h_

#include "safe_common.h"
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    const char *s__;
    size_t n__;
} SAFEV;

// Constructs an empty view.
LIBSAFE_INHEADER SAFEV SAFEV_new_empty(void)
{
    return (SAFEV) {.s__ = NULL, .n__ = 0};
}

// Constructs a view of a buffer by pointer and length.
LIBSAFE_INHEADER SAFEV SAFEV_new_UNSAFE(const char *ptr, size_t n)
{
    return (SAFEV) {
        .s__ = ptr,
        .n__ = n,
    };
}

// Constructs a view of a C string.
LIBSAFE_INHEADER SAFEV SAFEV_new_from_cstr_UNSAFE(const char *cstr)
{
    LIBSAFE_ASSERT(cstr != NULL);
    return (SAFEV) {
        .s__ = cstr,
        .n__ = strlen(cstr),
    };
}

#define SAFEV_new_from_literal(StrLit_) SAFEV_new_from_cstr_UNSAFE("" StrLit_)

// A static initializer for SAFEV (from a pointer and length).
#define SAFEV_STATIC_INIT_UNSAFE(Ptr_, Len_) {.s__ = (Ptr_), .n__ = (Len_)}

// A static initializer for SAFEV (from a string literal).
#define SAFEV_STATIC_INIT_FROM_LITERAL(StrLit_) \
    { \
        .s__ = "" StrLit_, \
        .n__ = sizeof(StrLit_) - 1, \
    }

// Peeks at a byte of a view.
LIBSAFE_INHEADER char SAFEV_at(SAFEV v, size_t i)
{
    LIBSAFE_ASSERT(i < v.n__);
    return v.s__[i];
}

// Peeks at a byte of a view, or, if index is invalid, return 'alt'.
LIBSAFE_INHEADER char SAFEV_at_or(SAFEV v, size_t i, char alt)
{
    if (i >= v.n__) {
        return alt;
    }
    return v.s__[i];
}

// Searches a view for a first occurence of character 'c'.
// If found, returns the index.
// If not found, returns (size_t) -1.
LIBSAFE_INHEADER size_t SAFEV_index_of(SAFEV v, unsigned char c)
{
    const char *pos = v.n__ ? memchr(v.s__, c, v.n__) : NULL;
    return pos ? (size_t) (pos - v.s__) : (size_t) -1;
}

// Checks if a view 'v' starts with a view 'prefix'.
LIBSAFE_INHEADER bool SAFEV_starts_with(SAFEV v, SAFEV prefix)
{
    if (prefix.n__ > v.n__) {
        return false;
    }
    if (!prefix.n__) {
        return true;
    }
    return memcmp(prefix.s__, v.s__, prefix.n__) == 0;
}

// Checks if a view 'v' equals to a view 'v1'.
LIBSAFE_INHEADER bool SAFEV_equals(SAFEV v, SAFEV v1)
{
    if (v.n__ != v1.n__) {
        return false;
    }
    if (!v.n__) {
        return true;
    }
    return memcmp(v.s__, v1.s__, v.n__) == 0;
}

// Constructs a subview of a view; 'i' is the starting index (inclusive), 'j' is the
// ending index (exclusive).
LIBSAFE_INHEADER SAFEV SAFEV_subspan(SAFEV v, size_t i, size_t j)
{
    LIBSAFE_ASSERT(i <= j);
    LIBSAFE_ASSERT(j <= v.n__);
    return (SAFEV) {
        .s__ = v.s__ + i,
        .n__ = j - i,
    };
}

// Equivalent to 'SAFEV_subspan(v, from_idx, SAFEV_len(v))'.
LIBSAFE_INHEADER SAFEV SAFEV_suffix(SAFEV v, size_t from_idx)
{
    LIBSAFE_ASSERT(from_idx <= v.n__);
    return (SAFEV) {
        .s__ = v.s__ + from_idx,
        .n__ = v.n__ - from_idx,
    };
}

// Returns the pointer of a view.
LIBSAFE_INHEADER const char *SAFEV_ptr_UNSAFE(SAFEV v)
{
    return v.s__;
}

// Returns the length of a view.
LIBSAFE_INHEADER size_t SAFEV_len(SAFEV v)
{
    return v.n__;
}

// If 'v' ends with 'c', strips it off; otherwise, returns it unchanged.
LIBSAFE_INHEADER SAFEV SAFEV_rstrip_once(SAFEV v, char c)
{
    if (!v.n__) {
        return v;
    }
    if (SAFEV_at(v, v.n__ - 1) != c) {
        return v;
    }
    return SAFEV_subspan(v, 0, v.n__ - 1);
}

// Returns min(i, SAFEV_len(v)).
LIBSAFE_INHEADER size_t SAFEV_trim_to_len(SAFEV v, size_t i)
{
    if (i > v.n__) {
        i = v.n__;
    }
    return i;
}

// This is needed from 'SAFEV_FMT_ARG': returns
//   min(bound, SAFEV_len(v)),
// assuming bound >= 0.
LIBSAFE_INHEADER int SAFEV_bounded_len(SAFEV v, int bound)
{
    LIBSAFE_ASSERT(bound >= 0);
    size_t res = v.n__;
    if (res > (unsigned) bound) {
        res = bound;
    }
    return res;
}

// Print a SAFEV as
//
//   void print_sv(SAFEV v)
//   {
//     printf("v = '%.*s'\n", SAFEV_FMT_ARG(v, 1024));
//   }
#define SAFEV_FMT_ARG(V_, MaxLen_) \
    SAFEV_bounded_len((V_), (MaxLen_)), \
    (V_).s__

#endif
