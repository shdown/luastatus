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

#pragma once

#include <lua.h>
#include <stddef.h>

// Pushes a pointer-and-length string onto the L's stack. Panics on
// out-of-memory condition instead of throwing like 'lua_pushlstring()'.
void ls_lua_madness_pushlstr(lua_State *L, const char *b, size_t nb);

// Pushes a C string onto the L's stack. Panics on out-of-memory condition
// instead of throwing like 'lua_pushstring()'.
//
// For compatibility with Lua's 'lua_pushstring': if 's' is NULL, pushes nil.
void ls_lua_madness_pushstr(lua_State *L, const char *s);

// Calls a function, like 'lua_call()', but panics on any error instead of
// throwing like 'lua_call()'.
void ls_lua_madness_call_or_die(lua_State *L, int nargs, int nresults);
