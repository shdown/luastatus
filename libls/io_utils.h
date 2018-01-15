#ifndef ls_io_utils_h_
#define ls_io_utils_h_

#include <fcntl.h>

#include "compdep.h"

// Makes a file descriptor close-on-exec.
// On success, /fd/ is returned.
// On failure, /-1/ is returned and /errno/ is set.
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

// Makes a file descriptor non-blocking.
// On success, /fd/ is returned.
// On failure, /-1/ is returned and /errno/ is set.
LS_INHEADER
int
ls_make_nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        return -1;
    }
    return fd;
}

#endif
