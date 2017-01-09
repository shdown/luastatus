#ifndef ls_io_utils_h_
#define ls_io_utils_h_

#include <stddef.h>
#include <sys/types.h>
#include <fcntl.h>
#include "compdep.h"

ssize_t
ls_full_read_append(int fd, char **pbuf, size_t *psize, size_t *palloc);

pid_t
ls_spawnp_pipe(const char *file, int *pipe_fd, char *const *argv);

LS_INHEADER
int
ls_make_cloexec(int fd)
{
    int flags = fcntl(fd, F_GETFD);
    if (flags < 0 || fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0) {
        return -1;
    }
    return fd;
}

#endif
