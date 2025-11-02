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

void ls_panic__log(const char *msg, const char *file, int line)
{
    fprintf(stderr, "LS_PANIC() at %s:%d: %s\n", file, line, msg);
}

void ls_panic_with_errnum__log(const char *msg, int errnum, const char *file, int line)
{
    char buf[512];
    fprintf(
        stderr, "LS_PANIC_WITH_ERRNUM() at %s:%d: %s: %s\n",
        file, line, msg, ls_strerror_r(errnum, buf, sizeof(buf)));
}

void ls_pth_check__log_and_abort(int ret, const char *expr, const char *file, int line)
{
    char buf[512];
    fprintf(
        stderr, "LS_PTH_CHECK(%s) failed at %s:%d, reason: %s\nAborting.\n",
        expr, file, line, ls_strerror_r(ret, buf, sizeof(buf)));
    abort();
}
