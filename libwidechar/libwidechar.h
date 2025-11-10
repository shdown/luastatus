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

#ifndef libwidechar_h_
#define libwidechar_h_

#include <stddef.h>
#include <lua.h>

size_t libwidechar_width(const char *s, size_t ns);

size_t libwidechar_truncate_to_width(
        const char *s, size_t ns,
        size_t max_width,
        size_t *out_resut_width);

void libwidechar_make_valid_and_printable(
        const char *s, size_t ns,
        const char *bad, size_t bad_len,
        void (*append)(void *ud, const char *ptr, size_t len),
        void *append_ud);

void libwidechar_register_lua_funcs(lua_State *L);

#endif
