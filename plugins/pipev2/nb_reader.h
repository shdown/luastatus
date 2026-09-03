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

#include <stddef.h>

struct NB_Reader;
typedef struct NB_Reader NB_Reader;

typedef struct {
    const char *ptr;
    size_t len;
} NB_Reader_Line;

NB_Reader *nb_reader_new(int fd, char delim);

// Return value:
//     -1:
//         I/O error or EOF; error number is in errno (if errno == 0, it's EOF).
//         Partial line content is written to /*out_line/.
//
//     0:  no whole line yet.
//
//     1:  got whole line.
//         Line content, without delimiter, is written to /*out_line/.
//         Call /nb_reader_consumed_line()/ when you don't need the line data anymore
//         (MUST be called before the next call to this function).
//
int nb_reader_read(NB_Reader *x, NB_Reader_Line *out_line);

void nb_reader_consumed_line(NB_Reader *x);

void nb_reader_destroy(NB_Reader *x);
