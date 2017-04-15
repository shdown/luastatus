#ifndef ls_wakeup_fifo_h_
#define ls_wakeup_fifo_h_

#include <time.h>
#include <signal.h>
#include <sys/select.h>

typedef struct {
    const char *fifo;
    const struct timespec *timeout;
    const sigset_t *sigmask;

    fd_set fds;
    int fd;
} LSWakeupFifo;

void
ls_wakeup_fifo_init(LSWakeupFifo *w);

int
ls_wakeup_fifo_open(LSWakeupFifo *w);

int
ls_wakeup_fifo_pselect(LSWakeupFifo *w);

void
ls_wakeup_fifo_destroy(LSWakeupFifo *w);

#endif
