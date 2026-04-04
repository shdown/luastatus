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

#include "urldecode.h"
#include <stddef.h>
#include <stdbool.h>
#include "libls/ls_panic.h"

size_t urldecode_size(SAFEV src)
{
    size_t n = SAFEV_len(src);
    size_t res = 0;

    size_t i = 0;
    while (i < n) {
        if (SAFEV_at(src, i) == '%') {
            i += 3;
        } else {
            ++i;
        }
        ++res;
    }

    return res;
}

static int parse_hex_digit(char c)
{
    if ('0' <= c && c <= '9') {
        return c - '0';
    }
    if ('a' <= c && c <= 'f') {
        return c - 'a' + 10;
    }
    if ('A' <= c && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

bool urldecode(SAFEV src, MUT_SAFEV dst)
{
    size_t n = SAFEV_len(src);

    size_t i = 0; // position in /src/
    size_t j = 0; // position in /dst/
    while (i < n) {
        char c = SAFEV_at(src, i);
        char new_c;
        if (c == '%') {
            if (i + 2 >= n) {
                return false;
            }
            int hi = parse_hex_digit(SAFEV_at(src, i + 1));
            int lo = parse_hex_digit(SAFEV_at(src, i + 2));
            if (hi < 0 || lo < 0) {
                return false;
            }
            new_c = (hi << 4) | lo;
            i += 3;
        } else {
            new_c = (c == '+') ? ' ' : c;
            ++i;
        }
        MUT_SAFEV_set_at(dst, j, new_c);
        ++j;
    }

    size_t expected_result_size = SAFEV_len(MUT_SAFEV_TO_SAFEV(dst));
    LS_ASSERT(j == expected_result_size);

    return true;
}
