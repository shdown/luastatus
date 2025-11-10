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

#ifndef luastatus_runshell_h_
#define luastatus_runshell_h_

#include <lua.h>

// This function is like /system()/, but:
//
//   1. Does not support /cmd == NULL/: on a POSIX system, /system(NULL)/ must
//      return non-zero anyway;
//
//   2. Assumes no thread cancellation takes place, and is not a cancellation
//      point (nobody is going to cancel luastatus' widget threads);
//
//   3. Does not modify signal dispositions for SIGINT and/or SIGQUIT (we are
//      not going to modify disposition for these signals anyway);
//
//   4. Is otherwise thread-safe (POSIX does not guarantee thread-safety of
//      /system()/, and, on musl, it is not thread-safe).
int runshell(const char *cmd);

int runshell_l_os_execute(lua_State *L);

int runshell_l_os_execute_lua51ver(lua_State *L);

#endif
