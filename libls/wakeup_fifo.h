#ifndef ls_wakeup_fifo_h_
#define ls_wakeup_fifo_h_

#include <time.h>
#include <signal.h>
#include <sys/select.h>

// An interface to perform a timed wait on a FIFO.
// Usage:
//
//     LSWakeupFifo w;
//     ls_wakeup_fifo_init(
//         &w,
//         fifo,       // /NULL/ means do not wait for FIFO events.
//         timeout,    // /ls_timespec_invalid/ means do not wait for timeout events.
//         sigmask     // /NULL/ means ignore all signals.
//     );
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
    fd_set fds_;
    int fd_;
} LSWakeupFifo;

void
ls_wakeup_fifo_init(LSWakeupFifo *w, const char *fifo, struct timespec timeout, sigset_t *sigmask);

int
ls_wakeup_fifo_open(LSWakeupFifo *w);

int
ls_wakeup_fifo_wait(LSWakeupFifo *w);

void
ls_wakeup_fifo_destroy(LSWakeupFifo *w);

#endif
