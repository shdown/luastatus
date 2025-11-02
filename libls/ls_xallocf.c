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

#include "ls_xallocf.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include "ls_alloc_utils.h"
#include "ls_panic.h"

char *ls_xallocvf(const char *fmt, va_list vl)
{
    va_list vl2;
    va_copy(vl2, vl);

    int n = vsnprintf(NULL, 0, fmt, vl);
    if (n < 0) {
        goto fail;
    }
    size_t nbuf = ((size_t) n) + 1;
    char *r = LS_XNEW(char, nbuf);
    if (vsnprintf(r, nbuf, fmt, vl2) < 0) {
        goto fail;
    }
    va_end(vl2);
    return r;

fail:
    LS_PANIC_WITH_ERRNUM("ls_xallocvf: vsnprintf() failed", errno);
}
