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

#include "escape_json_str.h"
#include <stddef.h>
#include "libsafe/safev.h"
#include "libsafe/mut_safev.h"

static const SAFEV HEX_CHARS = SAFEV_STATIC_INIT_FROM_LITERAL("0123456789ABCDEF");

static inline void append_sv(LS_String *dst, SAFEV v)
{
    ls_string_append_b(dst, SAFEV_ptr_UNSAFE(v), SAFEV_len(v));
}

void append_json_escaped_str(LS_String *dst, SAFEV v)
{
    ls_string_append_c(dst, '"');

    char esc_arr[] = {'\\', 'u', '0', '0', '#', '#'};
    MUT_SAFEV esc = MUT_SAFEV_new_UNSAFE(esc_arr, sizeof(esc_arr));

    size_t n = SAFEV_len(v);
    size_t prev = 0;
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = SAFEV_at(v, i);
        if (c < 32 || c == '\\' || c == '"' || c == '/') {
            append_sv(dst, SAFEV_subspan(v, prev, i));
            MUT_SAFEV_set_at(esc, 4, SAFEV_at(HEX_CHARS, c / 16));
            MUT_SAFEV_set_at(esc, 5, SAFEV_at(HEX_CHARS, c % 16));
            append_sv(dst, MUT_SAFEV_TO_SAFEV(esc));
            prev = i + 1;
        }
    }
    append_sv(dst, SAFEV_subspan(v, prev, n));

    ls_string_append_c(dst, '"');
}
