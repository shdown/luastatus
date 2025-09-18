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

#ifndef priv_h_
#define priv_h_

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

#include "libls/ls_string.h"

typedef struct {
    size_t nwidgets;

    LS_String *bufs;

    // Temporary buffer for secondary buffering, to avoid unneeded redraws.
    LS_String tmpbuf;

    // Input file descriptor.
    int in_fd;

    // /fdopen/'ed output file descritor.
    FILE *out;

    bool noclickev;

    bool noseps;
} Priv;

#endif
