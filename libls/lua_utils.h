/*
 * Copyright (C) 2015-2020  luastatus developers
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

#ifndef ls_lua_utils_h_
#define ls_lua_utils_h_

#include <lua.h>

// Traverse a table.
// Before using /break/ in this cycle, call /LS_LUA_BREAK(L_)/.
#define LS_LUA_TRAVERSE(L_, StackIndex_) \
    for (lua_pushnil(L_); \
         /* if /StackIndex_/ is relative to the top, decrease it by one because the previous key
          * will be pushed onto the stack each time lua_next() is called. */ \
         lua_next(L_, (StackIndex_) - ((StackIndex_) < 0 ? 1 : 0)); \
         lua_pop(L_, 1))

// Stack index of the key when in a /LS_LUA_TRAVERSE/ cycle.
#define LS_LUA_KEY   (-2)
// Stack index of the value when in a /LS_LUA_TRAVERSE/ cycle.
#define LS_LUA_VALUE (-1)
// Call before using /break/ to leave a /LS_LUA_TRAVERSE/ cycle.
#define LS_LUA_BREAK(L_) lua_pop(L_, 2)

#endif
