#ifndef ls_wakeup_fifo_h_
#define ls_wakeup_fifo_h_

#include <time.h>
#include <signal.h>
#include <sys/select.h>

// An interface to perform a timed wait on a FIFO.
// Usage:
//
//     LSWakeupFifo w;
//     if (ls_wakeup_fifo_init(&w) < 0) {
//         /* ... (errno is set) */
//         goto error;
//     }
//     w.fifo    = /* ... (NULL means don't wait for FIFO events) */;
//     w.timeout = /* ... (ls_timespec_invalid means don't wait for timeout events) */;
//     w.sigmask = /* ... (usually you don't need to do this) */;
//
//     /* ... */
//
//     while (1) {
//
//         /* ... */
//
//         if (ls_wakeup_fifo_open(&w) < 0) {
//             /* ... (errno is set) */
//             goto error;
//         }
//         switch (ls_wakeup_fifo_wait(&w)) {
//         case -1:
//             /* ... (errno is set) */
//             goto error;
//             break;
//         case 0:
//             /* ... (timeout) */
//             break;
//         case 1:
//         default:
//             /* ... (FIFO has been touched) */
//             break;
//         }
//     }
//
//     /* ... */
//
// error:
//     ls_wakeup_fifo_destroy(&w);
//     /* ... */

typedef struct {
    const char *fifo;
    struct timespec timeout;
    sigset_t sigmask;

    fd_set _fds;
    int _fd;
} LSWakeupFifo;

int
ls_wakeup_fifo_init(LSWakeupFifo *w);

int
ls_wakeup_fifo_open(LSWakeupFifo *w);

int
ls_wakeup_fifo_wait(LSWakeupFifo *w);

void
ls_wakeup_fifo_destroy(LSWakeupFifo *w);

#endif
