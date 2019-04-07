#ifndef ls_evloop_utils_h_
#define ls_evloop_utils_h_

#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <lua.h>
#include <signal.h>
#include <sys/select.h>

#include "compdep.h"

// Some plugins provide a "push_timeout"/"push_period" function that allows a widget to specify the
// next timeout for an otherwise constant timeout-based plugin's event loop.
//
// The pushed timeout value has to be guarded with a lock as the "push_timeout"/"push_period"
// function may be called from the /event/ function of a widget (that is, asynchronously from the
// plugin's viewpoint).
//
// /LSPushedTimeout/ is a structure containing the pushed timeout value (or an absence of such, as
// indicated by the value being /ls_timespec_invalid/), and a lock.
//
// <!!!>
// This structure must reside at a constant address throughout its whole life; this is required for
// the Lua closure created with /ls_pushed_timeout_push_luafunc()/.
// </!!!>
typedef struct {
    struct timespec value;
    pthread_spinlock_t lock;
} LSPushedTimeout;

// Initializes /p/ with an absence of pushed timeout value and a newly-created lock.
void
ls_pushed_timeout_init(LSPushedTimeout *p);

// Does the following actions atomically:
// * checks if /p/ has a pushed timeout value;
//   * if it does, clears it and returns the timeout value that /p/ has previously had;
//   * if it does not, returns /alt/.
struct timespec
ls_pushed_timeout_fetch(LSPushedTimeout *p, struct timespec alt);

// Creates a "push_timeout" function (a "C closure" with /p/'s address, in Lua terminology) on /L/'s
// stack.
//
// The resulting Lua function takes one numeric argument (the timeout in seconds) and does not
// return anything. It may throw an error if an invalid argument is provided. Once called, it will
// (atomically) alter /p/'s timeout value.
//
// The caller must ensure that the /L/'s stack has at least 2 free slots.
void
ls_pushed_timeout_push_luafunc(LSPushedTimeout *p, lua_State *L);

// Destroys /p/.
void
ls_pushed_timeout_destroy(LSPushedTimeout *p);

// Some plugins provide a so-called self-pipe facility; that is, the ability to "wake up" the
// widget's event loop (and force a call to the widget's /cb()/ function) from within the widget's
// /event()/ function via a special "wake_up" Lua function.
//
// /LSSelfPipe/ is a structure containing the file descriptors of the self-pipe.
// The /fds[0]/ file descriptor is to be read from, and /fds[1]/ is to be written to.
//
// <!!!>
// This structure must reside at a constant address throughout its whole life; this is required for
// the Lua closure created with /ls_self_pipe_push_luafunc()/.
// </!!!>
typedef struct {
    int fds[2];
} LSSelfPipe;

// The initializer for an empty (not yet opened) /LSSelfPipe/ structure.
#define LS_SELF_PIPE_NEW() {{-1, -1}}

// Opens a self-pipe /s/.
//
// On success, /0/ is returned; on failure, /-1/ is returned and /errno/ is set.
int
ls_self_pipe_open(LSSelfPipe *s);

// Creates a "wake_up" function (a "C closure" with /s/'s address, in Lua terminology) on /L/'s
// stack.
//
// The resulting Lua function takes no arguments and does not return anything. Once called, it will
// write a single byte to /s->fds[1]/ (in a thread-safe manner), or throw an error if the self-pipe
// has not been opened.
//
// The caller must ensure that the /L/'s stack has at least 2 free slots.
void
ls_self_pipe_push_luafunc(LSSelfPipe *s, lua_State *L);

// Checks whether /s/ has been opened.
LS_INHEADER
bool
ls_self_pipe_is_opened(LSSelfPipe *s)
{
    return s->fds[0] >= 0;
}

// If /s/ has been opened, closes it. After this function is called, /s/ must not be used anymore.
void
ls_self_pipe_close(LSSelfPipe *s);

// A device to perform a timed wait on a FIFO (although both the FIFO and timeout are optional).
// The typical usage is following:
//
//      LSWakeupFifo w;
//      ls_wakeup_fifo_init(&w, fifo, NULL);
//
//      // ...
//
//      while (1) {
//          // ...
//
//          if (ls_wakeup_fifo_open(&w) < 0) {
//              // ... (errno is set)
//          }
//          switch (ls_wakeup_fifo_wait(&w, timeout)) {
//          case -1:
//              // ... (errno is set)
//              goto error;
//          case 0:
//              // ... (timeout)
//              break;
//          case 1:
//              // ... (FIFO has been touched)
//              break;
//          }
//      }
//      // ...
//  error:
//      // ...
//      ls_wakeup_fifo_destroy(&w);
//
//
typedef struct {
    const char *fifo;
    sigset_t sigmask;
    int fifo_fd;
    fd_set fds;
} LSWakeupFifo;

// Initializes /w/ to use the FIFO /fifo/ (/NULL/ means no FIFO) and the signal mask /sigmask/,
// which specifies which signals to block during the wait (/NULL/ means all signals).
//
// If /fifo/ is not /NULL/, then it (as a pointer) must not be invalidated throughout the whole
// lifetime of /w/.
void
ls_wakeup_fifo_init(LSWakeupFifo *w, const char *fifo, sigset_t *sigmask);

// If /w/ has been configured to use a FIFO, and /w/'s FIFO file descriptor is not opened, opens
// it.
//
// On success, /0/ is returned; on failure, /-1/ is returned and /errno/ is set.
int
ls_wakeup_fifo_open(LSWakeupFifo *w);

// Blocks until either:
//
// * the FIFO is touched (if /w/ was configured to use the FIFO, and it was opened); the FIFO is
//   then closed and /1/ is returned;
//
// * /timeout/ elapses (if not equal to /ls_timespec_invalid/); in this case, /0/ is returned;
//
// * an error (including /EINTR/ if /w/ was configured to use a non-full signal mask) occurs; in
//   this case, /-1/ is returned and /errno/ is set.
int
ls_wakeup_fifo_wait(LSWakeupFifo *w, struct timespec timeout);

// Destroys /w/. After this function is called, /w/ must not be used anymore.
void
ls_wakeup_fifo_destroy(LSWakeupFifo *w);

#endif
