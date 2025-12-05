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

#ifndef safe_haven_h_
#define safe_haven_h_

#include <stdio.h>
#include <stdbool.h>
#include "libls/ls_strarr.h"
#include "libsafe/safev.h"

typedef enum {
    RESP_OK,
    RESP_ACK,
    RESP_OTHER,
} ResponseType;

bool is_good_greeting(SAFEV v);

ResponseType response_type(SAFEV v);

void write_quoted(FILE *f, SAFEV v);

// If /line/ is of form "key: value", appends /key/ and /value/ to /sa/.
void append_line_to_kv_strarr(LS_StringArray *sa, SAFEV line);

#endif
