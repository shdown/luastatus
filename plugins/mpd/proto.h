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

#ifndef proto_h_
#define proto_h_

#include <string.h>
#include <stdio.h>

#include "libls/ls_compdep.h"
#include "libls/ls_cstring_utils.h"

typedef enum {
    RESP_OK,
    RESP_ACK,
    RESP_OTHER,
} ResponseType;

LS_INHEADER ResponseType response_type(const char *s)
{
    if (strcmp(s, "OK\n") == 0) {
        return RESP_OK;
    }
    if ((ls_strfollow(s, "ACK "))) {
        return RESP_ACK;
    }
    return RESP_OTHER;
}

LS_INHEADER void write_quoted(FILE *f, const char *s)
{
    fputc('"', f);
    for (const char *t; (t = strchr(s, '"'));) {
        fwrite(s, 1, t - s, f);
        fputs("\\\"", f);
        s = t + 1;
    }
    fputs(s, f);
    fputc('"', f);
}

#endif
