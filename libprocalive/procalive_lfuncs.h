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

#ifndef libprocalive_procalive_lfuncs_h_
#define libprocalive_procalive_lfuncs_h_

#include <lua.h>

// Lua function access(path):
//     Checks if a given path exists, as if with 'access(path, F_OK)'. If it does
//     exist, returns 'true, nil'. If it does not, returns 'false, nil'. If an error
//     occurs, returns 'false, err_msg'.
int procalive_lfunc_access(lua_State *L);

// Lua function stat(path):
//     Tries to get the type of the file at the given path. On success returns either of:
//     * "regular",
//     * "dir" (directory),
//     * "chardev" (character device),
//     * "blockdev" (block device),
//     * "fifo",
//     * "symlink",
//     * "socket",
//     * "other".
// On failure returns '`'nil, err_msg'.
int procalive_lfunc_stat(lua_State *L);

// Lua function glob(pattern):
//     Performs glob expansion of pattern. A glob is a wildcard pattern like /tmp/*.txt
//     that can be applied as a filter to a list of existing files names. Supported
//     expansions are *, ? and [...]. Please refer to glob(7) for more information on
//     wildcard patterns.
//
//     Note also that the globbing is performed with GLOB_MARK flag, so that in output,
//     directories have trailing slash appended to their name.
//
//     Returns 'arr, nil' on success, where 'arr' is an array of strings; these are
//     existing file names that matched the given pattern. The order is arbitrary.
//
//     On failure, returns 'nil, err_msg'.
int procalive_lfunc_glob(lua_State *L);

// Lua function is_process_alive(pid):
//     Checks if a process with PID 'pid' is currently alive. 'pid' must be a number.
//     Returns a boolean that indicates whether the process is alive.
int procalive_lfunc_is_process_alive(lua_State *L);

// Registers all functions described above into a table on the top of /L/'s stack.
void procalive_lfuncs_register_all(lua_State *L);

#endif
