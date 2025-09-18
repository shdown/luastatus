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

#include "ls_parse_int.h"

#include <limits.h>
#include <errno.h>

int ls_strtou_b(const char *s, size_t ns, const char **endptr)
{
    int ret = 0;
    size_t i = 0;

    for (; i != ns; ++i) {
        int digit = ((int) s[i]) - '0';
        if (digit < 0 || digit > 9) {
            break;
        }
        if (ret > INT_MAX / 10) {
            ret = -ERANGE;
            break;
        }
        ret *= 10;
        if (ret > INT_MAX - digit) {
            ret = -ERANGE;
            break;
        }
        ret += digit;
    }

    if (endptr) {
        *endptr = s + i;
    }
    return ret;
}

int ls_full_strtou_b(const char *s, size_t ns)
{
    if (!ns) {
        return -EINVAL;
    }
    const char *endptr;
    int r = ls_strtou_b(s, ns, &endptr);
    if (r < 0) {
        return r;
    }
    if (endptr != s + ns) {
        return -EINVAL;
    }
    return r;
}
