/*
 * Copyright (C) 2026  luastatus developers
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

#include "urlencode.h"
#include <stdbool.h>
#include <stddef.h>
#include "libls/ls_panic.h"

static inline bool should_be_escaped(char c)
{
    if ('A' <= c && c <= 'Z') {
        return false;
    }

    if ('a' <= c && c <= 'z') {
        return false;
    }

    if ('0' <= c && c <= '9') {
        return false;
    }

    switch (c) {
    case '-': // minus
    case '_': // underscore
    case '.':
    case '~':
        return false;
    default:
        return true;
    }
}

size_t urlencode_size(SAFEV src, bool plus_notation)
{
    size_t n = SAFEV_len(src);
    size_t res = 0;
    for (size_t i = 0; i < n; ++i) {
        char c = SAFEV_at(src, i);
        size_t summand;
        if ((plus_notation && c == ' ') || !should_be_escaped(c)) {
            summand = 1;
        } else {
            summand = 3;
        }
        res += summand;
        if (res < summand) {
            // overflow
            return -1;
        }
    }
    return res;
}

static const SAFEV HEX_CHARS = SAFEV_STATIC_INIT_FROM_LITERAL("0123456789ABCDEF");

void urlencode(SAFEV src, MUT_SAFEV dst, bool plus_notation)
{
    size_t n = SAFEV_len(src);
    size_t j = 0; // position in /dst/

    for (size_t i = 0; i < n; ++i) {
        unsigned char c = SAFEV_at(src, i);
        if (plus_notation && c == ' ') {
            MUT_SAFEV_set_at(dst, j, '+');
            ++j;
        } else if (should_be_escaped(c)) {
            MUT_SAFEV_set_at(dst, j + 0, '%');
            MUT_SAFEV_set_at(dst, j + 1, SAFEV_at(HEX_CHARS, c / 16));
            MUT_SAFEV_set_at(dst, j + 2, SAFEV_at(HEX_CHARS, c % 16));
            j += 3;
        } else {
            MUT_SAFEV_set_at(dst, j, c);
            ++j;
        }
    }
    size_t expected_result_size = SAFEV_len(MUT_SAFEV_TO_SAFEV(dst));
    LS_ASSERT(j == expected_result_size);
}
