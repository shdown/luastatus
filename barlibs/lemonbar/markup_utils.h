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

#ifndef markup_utils_h_
#define markup_utils_h_

#include <lua.h>
#include <stddef.h>

#include "libls/ls_string.h"

void push_escaped(lua_State *L, const char *s, size_t ns);

void append_sanitized_b(LS_String *buf, size_t widget_idx, const char *s, size_t ns);

const char *parse_command(const char *line, size_t nline, size_t *ncommand, size_t *widget_idx);

#endif
