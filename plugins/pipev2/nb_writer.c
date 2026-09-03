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

#include "nb_writer.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include "libls/ls_alloc_utils.h"
#include "libls/ls_panic.h"
#include "libls/ls_io_utils.h"
#include "libls/ls_osdep.h"

int nb_writer_make_pipe(NB_Writer_Pipe *pipe)
{
    if (ls_cloexec_pipe(pipe->pipefds) < 0) {
        return -1;
    }
    ls_make_nonblock(pipe->pipefds[0]);
    ls_make_nonblock(pipe->pipefds[1]);
    return 0;
}

void nb_writer_pipe_destroy(NB_Writer_Pipe pipe)
{
    ls_close(pipe.pipefds[0]);
    ls_close(pipe.pipefds[1]);
}

typedef struct Node {
    struct Node *next;
    size_t ndata;
    char data[];
} Node;

struct NB_Writer {
    int fd;
    Node *first;
    Node *last;
    size_t first_write_pos;
    NB_Writer_Pipe pipe;
};

NB_Writer *nb_writer_new(int fd, NB_Writer_Pipe pipe)
{
    LS_ASSERT(fd >= 0);

    NB_Writer *x = LS_XNEW(NB_Writer, 1);
    *x = (NB_Writer) {
        .fd = fd,
        .first = NULL,
        .last = NULL,
        .first_write_pos = 0,
        .pipe = pipe,
    };
    return x;
}

void nb_writer_enqueue(NB_Writer *x, const char *chunk, size_t nchunk)
{
    LS_ASSERT(x != NULL);

    if (!nchunk) {
        return;
    }

    Node *node = ls_xmalloc(sizeof(Node) + nchunk, 1);
    node->next = NULL;
    node->ndata = nchunk;
    memcpy(node->data, chunk, nchunk);

    if (x->last) {
        x->last->next = node;
    } else {
        x->first = node;
        x->first_write_pos = 0;
    }
    x->last = node;

    ssize_t ignored = write(x->pipe.pipefds[1], "", 1);
    (void) ignored;
}

void nb_writer_get_fds_for_poll(
        NB_Writer *x,
        int *out_fd_pollout,
        int *out_fd_selfpipe)
{
    LS_ASSERT(x != NULL);

    *out_fd_pollout = (x->first != NULL) ? x->fd : -1;
    *out_fd_selfpipe = x->pipe.pipefds[0];
}

static void pop_first(NB_Writer *x)
{
    LS_ASSERT(x != NULL);

    Node *F = x->first;

    LS_ASSERT(F != NULL);

    x->first = F->next;
    if (!x->first) {
        x->last = NULL;
    }

    free(F);

    x->first_write_pos = 0;
}

int nb_writer_write(NB_Writer *x, bool woken_up_on_selfpipe)
{
    LS_ASSERT(x != NULL);

    if (woken_up_on_selfpipe) {
        char unused;
        ssize_t ignored = read(x->pipe.pipefds[0], &unused, 1);
        (void) ignored;
    }

    Node *F = x->first;

    LS_ASSERT(F != NULL);
    LS_ASSERT(x->first_write_pos < F->ndata);

    ssize_t w = write(
        x->fd,
        F->data  + x->first_write_pos,
        F->ndata - x->first_write_pos);
    if (w < 0) {
        if (LS_IS_EAGAIN(errno)) {
            return 0;
        }
        return -1;
    }
    x->first_write_pos += w;
    if (x->first_write_pos == F->ndata) {
        pop_first(x);
    }
    return 0;
}

void nb_writer_destroy(NB_Writer *x)
{
    LS_ASSERT(x != NULL);

    Node *cur = x->first;
    while (cur) {
        Node *next = cur->next;
        free(cur);
        cur = next;
    }

    nb_writer_pipe_destroy(x->pipe);

    free(x);
}
