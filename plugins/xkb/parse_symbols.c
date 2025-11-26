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

#include "parse_symbols.h"
#include <stddef.h>
#include "libsafe/safev.h"

void parse_symbols(
    SAFEV symbols,
    void (*append)(void *ud, SAFEV segment),
    void *ud)
{
    size_t n = SAFEV_len(symbols);

    size_t prev = 0;
    size_t balance = 0;
    for (size_t i = 0; i < n; ++i) {
        char c = SAFEV_at(symbols, i);
        switch (c) {
        case '(':
            ++balance;
            break;
        case ')':
            --balance;
            break;
        case '+':
            if (balance == 0) {
                append(ud, SAFEV_subspan(symbols, prev, i));
                prev = i + 1;
            }
            break;
        }
    }
    append(ud, SAFEV_suffix(symbols, prev));
}
