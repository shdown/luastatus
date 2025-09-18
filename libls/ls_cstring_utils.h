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

#ifndef ls_cstring_utils_h_
#define ls_cstring_utils_h_

#include <stddef.h>
#include <string.h>

#include "ls_compdep.h"

// If zero-terminated string /str/ starts with zero-terminated string /prefix/, returns
// /str + strlen(prefix)/; otherwise, returns /NULL/.
LS_INHEADER const char *ls_strfollow(const char *str, const char *prefix)
{
    size_t nprefix = strlen(prefix);
    return strncmp(str, prefix, nprefix) == 0 ? str + nprefix : NULL;
}

// Behaves like the GNU-specific /strerror_r/: either fills /buf/ and returns it, or returns a
// pointer to a static string.
const char *ls_strerror_r(int errnum, char *buf, size_t nbuf);

#endif
