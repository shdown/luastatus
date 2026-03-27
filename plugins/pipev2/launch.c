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

#include "launch.h"
#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <spawn.h>

#include "libls/ls_panic.h"
#include "libls/ls_osdep.h"

static inline void unmake_cloexec(int fd)
{
    int flags = fcntl(fd, F_GETFD);
    if (flags < 0) {
        LS_PANIC_WITH_ERRNUM("fcntl (F_GETFD) failed", errno);
    }
    flags &= ~FD_CLOEXEC;
    if (fcntl(fd, F_SETFD, flags) < 0) {
        LS_PANIC_WITH_ERRNUM("fcntl (F_SETFD) failed", errno);
    }
}

typedef struct {
    int fds[2];
    int their_end;
} Pipe;

#define PIPE_INIT_EMPTY {.fds = {-1, -1}, .their_end = -1}

static inline bool pipe_is_empty(Pipe *p)
{
    return p->their_end < 0;
}

static inline int pipe_make(Pipe *p, int their_end)
{
    if (ls_cloexec_pipe(p->fds) < 0) {
        return -1;
    }
    p->their_end = their_end;
    unmake_cloexec(p->fds[their_end]);
    return 0;
}

static inline int pipe_finalize(Pipe *p)
{
    if (pipe_is_empty(p)) {
        return -1;
    }
    close(p->fds[p->their_end]);

    // The following line sets /our_end/ to:
    //   * 1 if /p->their_end/ is 0;
    //   * 0 if /p->their_end/ is 1.
    int our_end = 1 ^ p->their_end;

    return p->fds[our_end];
}

static inline void pipe_destroy(Pipe *p)
{
    if (pipe_is_empty(p)) {
        return;
    }
    close(p->fds[0]);
    close(p->fds[1]);
}

static inline void add_redir(posix_spawn_file_actions_t *fa, Pipe *p)
{
    if (pipe_is_empty(p)) {
        return;
    }

    int fd_old = p->fds[p->their_end];
    int fd_new = p->their_end;

    if (fd_old == fd_new) {
        return;
    }
    LS_PTH_CHECK(posix_spawn_file_actions_adddup2(fa, fd_old, fd_new));
    LS_PTH_CHECK(posix_spawn_file_actions_addclose(fa, fd_old));
}

extern char **environ;

int launch(const LaunchArg *argv, bool redir_stdin, LaunchResult *res, LaunchError *err)
{
    LS_ASSERT(argv != NULL);
    LS_ASSERT(argv[0] != NULL);

    Pipe p_stdin = PIPE_INIT_EMPTY;
    Pipe p_stdout = PIPE_INIT_EMPTY;

    if (redir_stdin) {
        if (pipe_make(&p_stdin, /*their_end=*/ 0) < 0) {
            err->where = "pipe";
            err->errnum = errno;
            goto error;
        }
    }
    if (pipe_make(&p_stdout, /*their_end=*/ 1) < 0) {
        err->where = "pipe";
        err->errnum = errno;
        goto error;
    }

    posix_spawn_file_actions_t fa;
    LS_PTH_CHECK(posix_spawn_file_actions_init(&fa));

    add_redir(&fa, &p_stdin);
    add_redir(&fa, &p_stdout);

    pid_t pid;
    int rc = posix_spawnp(
        /*pid=*/ &pid,
        /*file=*/ argv[0],
        /*file_actions=*/ &fa,
        /*attrp=*/ NULL,
        /*argv=*/ argv,
        /*envp=*/ environ
    );

    LS_PTH_CHECK(posix_spawn_file_actions_destroy(&fa));

    if (rc != 0) {
        err->where = "posix_spawnp";
        err->errnum = rc;
        goto error;
    }

    int fd_stdin = pipe_finalize(&p_stdin);
    int fd_stdout = pipe_finalize(&p_stdout);

    *res = (LaunchResult) {
        .pid = pid,
        .fd_stdin = fd_stdin,
        .fd_stdout = fd_stdout,
    };
    return 0;

error:
    pipe_destroy(&p_stdin);
    pipe_destroy(&p_stdout);
    return -1;
}
