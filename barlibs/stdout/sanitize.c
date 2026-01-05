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

#include "sanitize.h"
#include <stddef.h>
#include "libls/ls_string.h"
#include "libsafe/safev.h"

static inline void append_sv(LS_String *dst, SAFEV v)
{
    ls_string_append_b(dst, SAFEV_ptr_UNSAFE(v), SAFEV_len(v));
}

void append_sanitized(LS_String *buf, SAFEV v)
{
    for (;;) {
        size_t i = SAFEV_index_of(v, '\n');
        if (i == (size_t) -1) {
            break;
        }
        append_sv(buf, SAFEV_subspan(v, 0, i));
        v = SAFEV_suffix(v, i + 1);
    }
    append_sv(buf, v);
}
