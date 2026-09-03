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
#include <stdbool.h>

//------------------------------------------------------------------------------

typedef struct {
    int pipefds[2];
} NB_Writer_Pipe;

#define NB_WRITER_PIPE_DUMMY_INIT {{-1, -1}}

int nb_writer_make_pipe(NB_Writer_Pipe *pipe);

void nb_writer_pipe_destroy(NB_Writer_Pipe pipe);

//------------------------------------------------------------------------------

struct NB_Writer;
typedef struct NB_Writer NB_Writer;

NB_Writer *nb_writer_new(int fd, NB_Writer_Pipe pipe);

void nb_writer_enqueue(NB_Writer *x, const char *chunk, size_t nchunk);

void nb_writer_get_fds_for_poll(
        NB_Writer *x,
        int *out_fd_pollout,
        int *out_fd_selfpipe);

int nb_writer_write(NB_Writer *x, bool woken_up_on_selfpipe);

void nb_writer_destroy(NB_Writer *x);
