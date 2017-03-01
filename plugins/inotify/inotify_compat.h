#ifndef inotify_compat_h_
#define inotify_compat_h_

#include <stdbool.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <errno.h>

#include "libls/io_utils.h"
#include "libls/osdep.h"

#include "probes.generated.h"

LS_INHEADER
int
compat_inotify_init(bool nonblock, bool cloexec)
{
#if HAVE_INOTIFY_INIT1
    return inotify_init1((nonblock ? IN_NONBLOCK : 0) |
                         (cloexec  ? IN_CLOEXEC  : 0));
#else
    int saved_errno;
    int fd = inotify_init();

    if (fd < 0) {
        goto error;
    }
    if (nonblock) {
        int fl = fcntl(fd, F_GETFL);
        if (fl < 0 || fcntl(fd, F_SETFL, fl | O_NONBLOCK) < 0) {
            goto error;
        }
    }
    if (cloexec) {
        if (ls_make_cloexec(fd) < 0) {
            goto error;
        }
    }
    return fd;

error:
    saved_errno = errno;
    ls_close(fd);
    errno = saved_errno;
    return -1;
#endif
}

#endif
