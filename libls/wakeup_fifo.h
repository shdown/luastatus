#ifndef ls_wakeup_fifo_h_
#define ls_wakeup_fifo_h_

#include <time.h>
#include <signal.h>
#include <sys/select.h>
#include "libls/compdep.h"

typedef struct {
    const char *_fifo;
    const struct timespec *_timeout;
    const sigset_t *_sigmask;

    fd_set _fds;
    int _fd;
} LSWakeupFifo;

void
ls_wakeup_fifo_init(LSWakeupFifo *w);

LS_INHEADER
void
ls_wakeup_fifo_setup(LSWakeupFifo *w, const char *fifo, const struct timespec *timeout,
                     const sigset_t *sigmask)
{
    w->_fifo = fifo;
    w->_timeout = timeout;
    w->_sigmask = sigmask;
}

int
ls_wakeup_fifo_open(LSWakeupFifo *w);

int
ls_wakeup_fifo_pselect(LSWakeupFifo *w);

void
ls_wakeup_fifo_destroy(LSWakeupFifo *w);

#endif
