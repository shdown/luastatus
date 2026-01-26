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

#ifndef open_stdio_file_
#define open_stdio_file_

#include <stdio.h>
#include <stdbool.h>
#include "include/barlib_data_v1.h"

bool open_input(
        LuastatusBarlibData *bd,
        FILE **dst,
        int fd,
        const char *filename);

bool open_output(
        LuastatusBarlibData *bd,
        FILE **dst,
        int fd);

#endif
