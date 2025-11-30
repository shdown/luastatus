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

#include "safe_haven.h"
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include "libls/ls_strarr.h"

#define LIT(Literal_) SAFEV_new_from_literal(Literal_)

bool is_good_greeting(SAFEV v)
{
    return SAFEV_starts_with(v, LIT("OK MPD "));
}

ResponseType response_type(SAFEV v)
{
    if (SAFEV_equals(v, LIT("OK"))) {
        return RESP_OK;
    }
    if (SAFEV_starts_with(v, LIT("ACK "))) {
        return RESP_ACK;
    }
    return RESP_OTHER;
}

static inline void do_fwrite_str_view(FILE *f, SAFEV v)
{
    size_t n = SAFEV_len(v);
    if (n) {
        fwrite(SAFEV_ptr_UNSAFE(v), 1, n, f);
    }
}

void write_quoted(FILE *f, SAFEV v)
{
    fputc('"', f);

    for (;;) {
        size_t i = SAFEV_index_of(v, '"');
        if (i == (size_t) -1) {
            break;
        }
        SAFEV cur_seg = SAFEV_subspan(v, 0, i);
        do_fwrite_str_view(f, cur_seg);
        fputs("\\\"", f);
        v = SAFEV_suffix(v, i + 1);
    }

    do_fwrite_str_view(f, v);

    fputc('"', f);
}

static inline void append_sv_to_strarr(LS_StringArray *sa, SAFEV v)
{
    ls_strarr_append(sa, SAFEV_ptr_UNSAFE(v), SAFEV_len(v));
}

void append_line_to_kv_strarr(LS_StringArray *sa, SAFEV line)
{
    size_t i = SAFEV_index_of(line, ':');
    if (i == (size_t) -1) {
        return;
    }
    if (SAFEV_at_or(line, i + 1, '\0') != ' ') {
        return;
    }

    SAFEV key = SAFEV_subspan(line, 0, i);
    append_sv_to_strarr(sa, key);

    SAFEV value = SAFEV_suffix(line, i + 2);
    append_sv_to_strarr(sa, value);
}
