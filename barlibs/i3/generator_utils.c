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

#include "generator_utils.h"

#include <math.h>
#include <stdbool.h>

void
append_json_escaped_s(LSString *s, const char *zts)
{
    ls_string_append_c(s, '"');
    size_t prev = 0;
    size_t i;
    for (i = 0; ; ++i) {
        unsigned char c = zts[i];
        if (c == '\0') {
            break;
        }
        if (c < 32 || c == '\\' || c == '"' || c == '/') {
            ls_string_append_b(s, zts + prev, i - prev);
            ls_string_append_f(s, "\\u%04X", c);
            prev = i + 1;
        }
    }
    ls_string_append_b(s, zts + prev, i - prev);
    ls_string_append_c(s, '"');
}

bool
append_json_number(LSString *s, double value)
{
    if (!isfinite(value)) {
        return false;
    }
    return ls_string_append_f(s, "%.20g", value);
}
