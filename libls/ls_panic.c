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

#include "ls_panic.h"
#include <stdlib.h>
#include <stdio.h>
#include "ls_cstring_utils.h"

void ls__do_panic_impl__(
    const char *macro_name,
    const char *expr,
    const char *msg,
    const char *func, const char *file, int line)
{
    fprintf(
        stderr, "%s(%s) failed in %s at %s:%d: %s\n",
        macro_name, expr,
        func, file, line,
        msg);
    abort();
}

void ls__do_panic_with_errnum_impl__(
    const char *macro_name,
    const char *expr,
    const char *msg,
    int errnum,
    const char *func, const char *file, int line)
{
    char buf[512];
    fprintf(
        stderr, "%s(%s) failed in %s at %s:%d: %s: %s\n",
        macro_name, expr,
        func, file, line,
        msg, ls_strerror_r(errnum, buf, sizeof(buf))
    );
    abort();
}
