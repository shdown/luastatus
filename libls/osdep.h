#ifndef ls_osdep_h_
#define ls_osdep_h_

// In no event should _GNU_SOURCE or something similar be defined here!

#include <unistd.h>
#include <errno.h>
#include "compdep.h"
#include "probes.generated.h"

// EINTR-safe (as possible) close(). Please read:
// 1. http://www.daemonology.net/blog/2011-12-17-POSIX-close-is-broken.html
// 2. https://news.ycombinator.com/item?id=3363819
// 3. https://sourceware.org/bugzilla/show_bug.cgi?id=16302
#if LS_HAVE_POSIX_CLOSE
#   define ls_close(FD_) posix_close(FD_, 0)
#elif defined (__hpux)
LS_INHEADER
int
ls_close(int fd)
{
    int r;
    while ((r = close(fd)) < 0 && errno == EINTR) {}
    return r;
}
#else
#   define ls_close close
#endif

// The behaviour is same as calling pipe(pipefd), except that both file descriptors are made
// CLOEXEC. If the latter fails, the pipe is destroyed, -1 is returned and errno is set.
int
ls_cloexec_pipe(int pipefd[2]);

// The behaviour is same as calling socket(domain, type, protocol), except that the file descriptor
// is make CLOEXEC. If the latter fails, the pipe is destroyed, -1 is reurned and errno is set.
int
ls_cloexec_socket(int domain, int type, int protocol);

#endif
