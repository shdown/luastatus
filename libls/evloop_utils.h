/*
 * Copyright (C) 2015-2020  luastatus developers
 *
 * This file is part of luastatus.
 *
 * luastatus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * luastatus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with luastatus.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ls_evloop_utils_h_
#define ls_evloop_utils_h_

#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <lua.h>
#include <errno.h>
#include <poll.h>

#include "compdep.h"
#include "cstring_utils.h"

// Some plugins provide a "push_timeout"/"push_period" function that allows a widget to specify the
// next timeout for an otherwise constant timeout-based plugin's event loop.
//
// The pushed timeout value has to be guarded with a lock as the "push_timeout"/"push_period"
// function may be called from the /event/ function of a widget (that is, asynchronously from the
// plugin's viewpoint).
//
// /LSPushedTimeout/ is a structure containing the pushed timeout value (or an absence of such, as
// indicated by the value being negative), and a lock.
//
// <!!!>
// This structure must reside at a constant address throughout its whole life; this is required for
// the Lua closure created with /ls_pushed_timeout_push_luafunc()/.
// </!!!>
typedef struct {
    double value;
    pthread_spinlock_t lock;
} LSPushedTimeout;

// Initializes /p/ with an absence of pushed timeout value and a newly-created lock.
void
ls_pushed_timeout_init(LSPushedTimeout *p);

// Does the following actions atomically:
// * checks if /p/ has a pushed timeout value;
//   * if it does, clears it and returns the timeout value that /p/ has previously had;
//   * if it does not, returns /alt/.
double
ls_pushed_timeout_fetch(LSPushedTimeout *p, double alt);

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

int ls_poll(struct pollfd *fds, nfds_t nfds, double tmo);

int ls_wait_input_on_fd(int fd, double tmo);

int ls_fifo_open(int *fd, const char *fifo);

#define LS_FIFO_STRERROR_ONSTACK(E_) \
    ((E_) == -EINVAL ? "Not a FIFO" : ls_strerror_onstack(E_))

int ls_fifo_wait(int *fd, double tmo);

#endif
