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

#include "markup_utils.h"

#include "libls/string_.h"
#include "libls/parse_int.h"

#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <lauxlib.h>

void push_escaped(lua_State *L, const char *s, size_t ns)
{
    // just replace all "%"s with "%%"

    luaL_Buffer b;
    luaL_buffinit(L, &b);
    // we have to check /ns/ before calling /memchr/, see DOCS/c_notes/empty-ranges-and-c-stdlib.md
    for (const char *t; ns && (t = memchr(s, '%', ns));) {
        size_t nseg = t - s + 1;
        luaL_addlstring(&b, s, nseg);
        luaL_addchar(&b, '%');
        ns -= nseg;
        s += nseg;
    }
    luaL_addlstring(&b, s, ns);

    luaL_pushresult(&b);
}

void append_sanitized_b(LSString *buf, size_t widget_idx, const char *s, size_t ns)
{
    size_t prev = 0;
    bool a_tag = false;
    for (size_t i = 0; i < ns; ++i) {
        switch (s[i]) {
        case '\n':
            ls_string_append_b(buf, s + prev, i - prev);
            prev = i + 1;
            break;

        case '%':
            if (i + 1 < ns) {
                if (s[i + 1] == '{' && i + 2 < ns && s[i + 2] == 'A') {
                    a_tag = true;
                } else if (s[i + 1] == '%') {
                    ++i;
                }
            }
            break;

        case ':':
            if (a_tag) {
                ls_string_append_b(buf, s + prev, i + 1 - prev);
                ls_string_append_f(buf, "%zu_", widget_idx);
                prev = i + 1;
                a_tag = false;
            }
            break;

        case '}':
            a_tag = false;
            break;
        }
    }
    ls_string_append_b(buf, s + prev, ns - prev);
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
