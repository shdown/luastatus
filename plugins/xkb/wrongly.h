/*
 * Copyright (C) 2015-2021  luastatus developers
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

#ifndef wrongly_h_
#define wrongly_h_

#include <X11/Xlib.h>
#include <stdbool.h>

#include "libls/strarr.h"

typedef struct {
    // These all are either zero-terminated strings allocated as if with /malloc()/, or /NULL/.
    char *rules;
    char *model;
    char *layout;
    char *options;
} WronglyResult;

bool wrongly_fetch(Display *dpy, WronglyResult *out);

void wrongly_parse_layout(const char *layout, LSStringArray *out);

#endif
