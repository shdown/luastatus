#include "io_utils.h"

#include <spawn.h>
#include <errno.h>
#include <unistd.h>
#include "alloc_utils.h"
#include "osdep.h"

extern char **environ;

ssize_t
ls_full_read_append(int fd, char **pbuf, size_t *psize, size_t *palloc)
{
    size_t nread = 0;
    while (1) {
        if (*psize == *palloc) {
            *pbuf = ls_x2realloc(*pbuf, palloc, 1);
        }
        ssize_t r = read(fd, *pbuf + *psize, *palloc - *psize);
        if (r < 0) {
            return -1;
        } else if (r == 0) {
            break;
        } else {
            *psize += r;
            nread += r;
        }
    }
    return nread;
}

pid_t
ls_spawnp_pipe(const char *file, int *pipe_fd, char *const *argv)
{
    pid_t pid = -1;
    int pipe_fds[2] = {-1, -1};
    posix_spawn_file_actions_t fa;
    // Contract: once fa has been initialized, p_inited_fa points to it.
    posix_spawn_file_actions_t *p_inited_fa = NULL;
    int saved_errno;

    if (pipe_fd) {
        if (ls_cloexec_pipe(pipe_fds) < 0) {
            pipe_fds[0] = pipe_fds[1] = -1;
            goto cleanup;
        }
        if ((errno = posix_spawn_file_actions_init(&fa))) {
            goto cleanup;
        }
        p_inited_fa = &fa;
        if ((errno = posix_spawn_file_actions_adddup2(p_inited_fa, pipe_fds[1], 1))) {
            goto cleanup;
        }
    }

    if ((errno = posix_spawnp(&pid, file, p_inited_fa, NULL, argv, environ))) {
        pid = -1;
        goto cleanup;
    }

    if (pipe_fd) {
        // "move out" value from pipe_fds[0] to *pipe_fd
        *pipe_fd = pipe_fds[0];
        pipe_fds[0] = -1;
    }

cleanup:
    saved_errno = errno;
    ls_close(pipe_fds[0]);
    ls_close(pipe_fds[1]);
    if (p_inited_fa) {
        posix_spawn_file_actions_destroy(p_inited_fa);
    }
    errno = saved_errno;
    return pid;
}
