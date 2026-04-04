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

#include "argparser.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

// If zero-terminated string /str/ starts with zero-terminated string /prefix/, returns
// /str + strlen(prefix)/; otherwise, returns /NULL/.
static const char *strfollow(const char *str, const char *prefix)
{
    size_t nprefix = strlen(prefix);
    return strncmp(str, prefix, nprefix) == 0 ? str + nprefix : NULL;
}

static bool parse_int(const char *s, int *dst)
{
    errno = 0;
    char *endptr;
    long res = strtol(s, &endptr, 10);
    if (errno != 0 || *s == '\0' || *endptr != '\0') {
        return false;
    }
#if LONG_MAX > INT_MAX
    if (res < INT_MIN || res > INT_MAX) {
        return false;
    }
#endif
    *dst = res;
    return true;
}

static ArgParser_Option *find_opt(ArgParser_Option *opts, const char *arg, const char **out_value)
{
    for (ArgParser_Option *cur = opts; cur->spelling; ++cur) {
        const char *value = strfollow(arg, cur->spelling);
        if (value) {
            *out_value = value;
            return cur;
        }
    }
    return NULL;
}

bool arg_parser_parse(
        ArgParser_Option *opts,
        char **pos, int npos,
        int argc, char **argv,
        char *errbuf, size_t nerrbuf)
{
    bool dash_dash_mode = false;
    int pos_idx = 0;

    for (int i = 1; i < argc; ++i) {
        char *arg = argv[i];

        if (arg[0] == '-' && !dash_dash_mode) {
            // This is either an option or a dash-dash.

            if (strcmp(arg, "--") == 0) {
                // This is a dash-dash.
                dash_dash_mode = true;
                continue;
            }

            // This is an option.
            const char *value;
            ArgParser_Option *found = find_opt(opts, arg, &value);
            if (!found) {
                snprintf(errbuf, nerrbuf, "unknown option '%s'", arg);
                return false;
            }
            if (!parse_int(value, found->dst)) {
                snprintf(errbuf, nerrbuf, "cannot parse value of '%s' option as int", arg);
                return false;
            }

        } else {
            // This is a position argument.
            if (pos_idx == npos) {
                snprintf(errbuf, nerrbuf, "too many positional arguments");
                return false;
            }
            pos[pos_idx++] = arg;
        }
    }

    if (pos_idx != npos) {
        snprintf(errbuf, nerrbuf, "not enough positional arguments");
        return false;
    }

    return true;
}
