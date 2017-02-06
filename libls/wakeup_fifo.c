#include "wakeup_fifo.h"

#include <errno.h>
#include <fcntl.h>
#include "osdep.h"

void
ls_wakeup_fifo_init(LSWakeupFifo *w)
{
    FD_ZERO(&w->fds);
    w->fd = -1;
}

int
ls_wakeup_fifo_open(LSWakeupFifo *w)
{
    if (w->fd < 0 && w->fifo) {
        w->fd = open(w->fifo, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (w->fd < 0) {
            return -1;
        }
        if (w->fd >= FD_SETSIZE) {
            ls_close(w->fd);
            w->fd = -1;
            errno = EMFILE;
            return -1;
        }
        return 0;
    }
    return 0;
}

int
ls_wakeup_fifo_pselect(LSWakeupFifo *w)
{
    // Contract: w->fds is either empty or only contains w->fd.
    const int fd = w->fd;

    if (fd >= 0) {
        FD_SET(fd, &w->fds);
    }
    const int r = pselect(fd >= 0 ? fd + 1 : 0, &w->fds, NULL, NULL, w->timeout, w->sigmask);
    if (r < 0) {
        int saved_errno = errno;
        ls_close(fd);
        w->fd = -1;
        errno = saved_errno;
        return -1;
    } else if (r == 0) {
        return 0;
    } else {
        ls_close(fd);
        w->fd = -1;
        return 1;
    }
}

void
ls_wakeup_fifo_destroy(LSWakeupFifo *w)
{
    ls_close(w->fd);
}
