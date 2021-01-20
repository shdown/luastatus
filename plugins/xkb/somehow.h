/*
 * Copyright (C) 2021  luastatus developers
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

#ifndef somehow_h_
#define somehow_h_

#include <X11/Xlib.h>
#include <stdint.h>

#include "libls/strarr.h"

// Returns either NULL or a NUL-terminated string allocated as if with /malloc()/.
char *somehow_fetch_symbols(Display *dpy, uint64_t deviceid);

void somehow_parse_symbols(const char *symbols, LSStringArray *out, const char *bad);

#endif
