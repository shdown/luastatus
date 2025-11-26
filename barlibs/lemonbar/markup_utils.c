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

#include "markup_utils.h"

#include "libls/ls_string.h"
#include "libls/ls_parse_int.h"
#include "libsafe/safev.h"

#include <stddef.h>
#include <stdbool.h>

void escape(
    void (*append)(void *ud, SAFEV segment),
    void *ud,
    SAFEV v)
{
    // just replace all "%"s with "%%"
    for (;;) {
        size_t i = SAFEV_index_of(v, '%');
        if (i == (size_t) -1) {
            break;
        }
        append(ud, SAFEV_subspan(v, 0, i));
        append(ud, SAFEV_new_from_literal("%%"));
        v = SAFEV_suffix(v, i + 1);
    }
    append(ud, v);
}

static inline void append_sv(LS_String *dst, SAFEV v)
{
    ls_string_append_b(dst, SAFEV_ptr_UNSAFE(v), SAFEV_len(v));
}

void append_sanitized(LS_String *buf, size_t widget_idx, SAFEV v)
{
    size_t n = SAFEV_len(v);

    size_t prev = 0;
    bool a_tag = false;
    for (size_t i = 0; i < n; ++i) {

#define DO_PREV(WhetherToIncludeThis_) \
    do { \
        size_t j__ = i + ((WhetherToIncludeThis_) ? 1 : 0); \
        append_sv(buf, SAFEV_subspan(v, prev, j__)); \
        prev = i + 1; \
    } while (0)

#define PEEK(Offset_) SAFEV_at_or(v, i + (Offset_), '\0')

        switch (SAFEV_at(v, i)) {
        case '\n':
            DO_PREV(false);
            break;

        case '%':
            if (PEEK(1) == '{' && PEEK(2) == 'A') {
                a_tag = true;
            } else if (PEEK(1) == '%') {
                ++i;
            }
            break;

        case ':':
            if (a_tag) {
                DO_PREV(true);
                ls_string_append_f(buf, "%zu_", widget_idx);
                a_tag = false;
            }
            break;

        case '}':
            a_tag = false;
            break;
        }

#undef DO_PREV
#undef PEEK

    }

    append_sv(buf, SAFEV_suffix(v, prev));
}

const char *parse_command(const char *line, size_t nline, size_t *ncommand, size_t *widget_idx)
{
    const char *endptr;
    int idx = ls_strtou_b(line, nline, &endptr);
    if (idx < 0 ||
        endptr == line ||
        endptr == line + nline ||
        *endptr != '_')
    {
        return NULL;
    }
    const char *command = endptr + 1;
    *ncommand = nline - (command - line);
    *widget_idx = idx;
    return command;
}
