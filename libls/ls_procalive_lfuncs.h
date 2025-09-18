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

#ifndef ls_procalive_lfuncs_h_
#define ls_procalive_lfuncs_h_

#include <lua.h>

int ls_procalive_lfunc_access(lua_State *L);

int ls_procalive_lfunc_stat(lua_State *L);

int ls_procalive_lfunc_glob(lua_State *L);

int ls_procalive_lfunc_is_process_alive(lua_State *L);

void ls_procalive_lfuncs_register_all(lua_State *L);

#endif
