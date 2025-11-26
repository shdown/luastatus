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

#ifndef line_reader_h_
#define line_reader_h_

#include <stddef.h>
#include <stdio.h>
#include "libsafe/safev.h"

typedef struct {
    char *buf;
    size_t capacity;
} LineReader;

LineReader line_reader_new(size_t prealloc);

int line_reader_read_line(LineReader *LR, FILE *f, SAFEV *out);

void line_reader_destroy(LineReader *LR);

#endif
