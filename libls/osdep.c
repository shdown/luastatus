#define _GNU_SOURCE
#include "osdep.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include "io_utils.h"
#include "probes.generated.h"

int
ls_close(int fd)
{
#if LS_HAVE_POSIX_CLOSE
    return posix_close(fd, 0);
#elif defined(__hpux)
    int r;
    while ((r = close(fd)) < 0 && errno == EINTR) {}
    return r;
#else
    return close(fd);
#endif
}

int
ls_cloexec_pipe(int pipefd[2])
{
#if LS_HAVE_GNU_PIPE2
    return pipe2(pipefd, O_CLOEXEC);
#else
    if (pipe(pipefd) < 0) {
        return -1;
    }
    for (int i = 0; i < 2; ++i) {
        if (ls_make_cloexec(pipefd[i]) < 0) {
            int saved_errno = errno;
            ls_close(pipefd[0]);
            ls_close(pipefd[1]);
            errno = saved_errno;
            return -1;
        }
    }
    return 0;
#endif
}

int
ls_cloexec_socket(int domain, int type, int protocol)
{
#if LS_HAVE_GNU_SOCK_CLOEXEC
    return socket(domain, type | SOCK_CLOEXEC, protocol);
#else
    int fd = socket(domain, type, protocol);
    if (fd < 0) {
        return -1;
    }
    if (ls_make_cloexec(fd) < 0) {
        int saved_errno = errno;
        ls_close(fd);
        errno = saved_errno;
        return -1;
    }
    return fd;
#endif
}
