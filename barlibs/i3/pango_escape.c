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

#include "pango_escape.h"
#include <stddef.h>
#include "libsafe/safev.h"

#define LIT(StrLit_) SAFEV_new_from_literal(StrLit_)

void pango_escape(
    SAFEV v,
    void (*append)(void *ud, SAFEV segment),
    void *ud)
{
    size_t n = SAFEV_len(v);
    size_t prev = 0;
    for (size_t i = 0; i < n; ++i) {
        SAFEV esc = {0};
        switch (SAFEV_at(v, i)) {
        case '&':  esc = LIT("&amp;");   break;
        case '<':  esc = LIT("&lt;");    break;
        case '>':  esc = LIT("&gt;");    break;
        case '\'': esc = LIT("&apos;");  break;
        case '"':  esc = LIT("&quot;");  break;
        default: continue;
        }
        append(ud, SAFEV_subspan(v, prev, i));
        append(ud, esc);
        prev = i + 1;
    }
    append(ud, SAFEV_suffix(v, prev));
}
