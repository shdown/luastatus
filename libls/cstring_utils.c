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

#if _POSIX_C_SOURCE < 200112L || defined(_GNU_SOURCE)
#   error "Unsupported feature test macros; either tune them or change the code."
#endif

#include "cstring_utils.h"

#include <string.h>

const char *ls_strerror_r(int errnum, char *buf, size_t nbuf)
{
    // We introduce an /int/ variable in order to get a compilation warning if /strerror_r()/ is
    // still GNU-specific and returns a pointer to char.
    int r = strerror_r(errnum, buf, nbuf);
    return r == 0 ? buf : "unknown error or truncated error message";
}
