#include "wakeup_fifo.h"

#include <errno.h>
#include <fcntl.h>
#include "osdep.h"

void
ls_wakeup_fifo_init(LSWakeupFifo *w)
{
    FD_ZERO(&w->_fds);
    w->_fd = -1;
    // Not really required, just in case a user forgets to set w->fifo.
    w->fifo = NULL;
}

int
ls_wakeup_fifo_open(LSWakeupFifo *w)
{
    if (w->_fd < 0 && w->fifo) {
        w->_fd = open(w->fifo, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (w->_fd < 0) {
            return -1;
        }
        if (w->_fd >= FD_SETSIZE) {
            ls_close(w->_fd);
            w->_fd = -1;
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
    const int fd = w->_fd;

    if (fd >= 0) {
        FD_SET(fd, &w->_fds);
    }
    const int r = pselect(fd >= 0 ? fd + 1 : 0, &w->_fds, NULL, NULL, w->timeout, w->sigmask);
    if (r < 0) {
        int saved_errno = errno;
        if (fd >= 0) {
            FD_CLR(fd, &w->_fds);
        }
        ls_close(fd);
        w->_fd = -1;
        errno = saved_errno;
        return -1;
    } else if (r == 0) {
        return 0;
    } else {
        FD_CLR(fd, &w->_fds);
        ls_close(fd);
        w->_fd = -1;
        return 1;
    }
}

void
ls_wakeup_fifo_destroy(LSWakeupFifo *w)
{
    ls_close(w->_fd);
}
