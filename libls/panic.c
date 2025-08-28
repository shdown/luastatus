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

#include "panic.h"
#include <stdlib.h>
#include "cstring_utils.h"

void ls_pth_check_impl(int ret, const char *expr, const char *file, int line)
{
    if (ret == 0) {
        return;
    }

    char buf[512];
    fprintf(stderr, "LS_PTH_CHECK(%s) failed at %s:%d, reason: %s\nAborting.\n",
            expr, file, line, ls_strerror_r(ret, buf, sizeof(buf)));
    abort();
}
