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

#include "ls_string.h"

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include "ls_panic.h"

void ls_string_append_vf(LS_String *x, const char *fmt, va_list vl)
{
    va_list vl2;
    va_copy(vl2, vl);

    size_t navail = x->capacity - x->size;
    int r = vsnprintf(x->data + x->size, navail, fmt, vl);
    if (r < 0) {
        goto fail;
    }
    if (((size_t) r) >= navail) {
        ls_string_ensure_avail(x, ((size_t) r) + 1);
        if (vsnprintf(x->data + x->size, ((size_t) r) + 1, fmt, vl2) < 0) {
            goto fail;
        }
    }
    x->size += r;
    va_end(vl2);
    return;

fail:
    LS_PANIC("ls_string_append_vf: vsnprintf() failed");
}
