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

#include "inject_all.h"
#include <lua.h>
#include "datestuff.h"
#include "osstuff.h"
#include "randstuff.h"
#include "runshell.h"

void liblreplace_inject_all(lua_State *L)
{
    liblreplace_datestuff_inject(L);
    liblreplace_osstuff_inject(L);
    liblreplace_randstuff_inject(L);
    liblreplace_runshell_inject(L);
}
