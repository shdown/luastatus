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

#ifndef ls_evloop_lfuncs_h_
#define ls_evloop_lfuncs_h_

#include <lua.h>
#include <lauxlib.h>
#include <pthread.h>
#include <unistd.h>
#include "compdep.h"
#include "panic.h"
#include "osdep.h"
#include "io_utils.h"

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
LS_INHEADER void ls_pushed_timeout_init(LSPushedTimeout *p)
{
    p->value = -1;
    LS_PTH_CHECK(pthread_spin_init(&p->lock, PTHREAD_PROCESS_PRIVATE));
}

// Does the following actions atomically:
// * checks if /p/ has a pushed timeout value;
//   * if it does, clears it and returns the timeout value that /p/ has previously had;
//   * if it does not, returns /alt/.
LS_INHEADER double ls_pushed_timeout_fetch(LSPushedTimeout *p, double alt)
{
    double r;
    pthread_spin_lock(&p->lock);
    if (p->value < 0) {
        r = alt;
    } else {
        r = p->value;
        p->value = -1;
    }
    pthread_spin_unlock(&p->lock);
    return r;
}

LS_INHEADER int ls_pushed_timeout_lfunc(lua_State *L)
{
    double arg = luaL_checknumber(L, 1);
    if (!(arg >= 0))
        return luaL_error(L, "invalid timeout");

    LSPushedTimeout *p = lua_touserdata(L, lua_upvalueindex(1));

    pthread_spin_lock(&p->lock);
    p->value = arg;
    pthread_spin_unlock(&p->lock);

    return 0;
}

// Creates a "push_timeout" function (a "C closure" with /p/'s address, in Lua terminology) on /L/'s
// stack.
//
// The resulting Lua function takes one numeric argument (the timeout in seconds) and does not
// return anything. It may throw an error if an invalid argument is provided. Once called, it will
// (atomically) alter /p/'s timeout value.
//
// The caller must ensure that the /L/'s stack has at least 2 free slots.
LS_INHEADER void ls_pushed_timeout_push_luafunc(LSPushedTimeout *p, lua_State *L)
{
    lua_pushlightuserdata(L, p);
    lua_pushcclosure(L, ls_pushed_timeout_lfunc, 1);
}

// Destroys /p/.
LS_INHEADER void ls_pushed_timeout_destroy(LSPushedTimeout *p)
{
    pthread_spin_destroy(&p->lock);
}

// Some plugins provide the so-called self-pipe facility; that is, the ability to "wake up" the
// widget's event loop (and force a call to the widget's /cb()/ function) from within the widget's
// /event()/ function via a special "wake_up" Lua function.
//
// Such a pipe consists of an array, /int fds[2]/, of two file descriptors; the /fds[0]/ file
// descriptor is to be read from, and /fds[1]/ is to be written to.
//
// <!!!>
// This array must reside at a constant address throughout its whole life; this is required for
// the Lua closure created with /ls_self_pipe_push_luafunc()/.
// </!!!>

// Creates a new self-pipe.
//
// On success, /0/ is returned.
//
// On failure, /-1/ is returned, /fds[0]/ and /fds[1]/ are set to -1, and /errno/ is set.
LS_INHEADER int ls_self_pipe_open(int fds[2])
{
    if (ls_cloexec_pipe(fds) < 0) {
        fds[0] = -1;
        fds[1] = -1;
        return -1;
    }
    ls_make_nonblock(fds[0]);
    ls_make_nonblock(fds[1]);
    return 0;
}

LS_INHEADER int ls_self_pipe_lfunc(lua_State *L)
{
    int *fds = lua_touserdata(L, lua_upvalueindex(1));

    int fd = fds[1];
    if (fd < 0)
        return luaL_error(L, "self-pipe has not been opened");

    ssize_t unused = write(fd, "", 1); // write '\0'
    (void) unused;

    return 0;
}

// Creates a "wake_up" function (a "C closure" with /s/'s address, in Lua terminology) on /L/'s
// stack.
//
// The resulting Lua function takes no arguments and does not return anything. Once called, it will
// write a single byte to /fds[1]/ (in a thread-safe manner), or throw an error if the self-pipe
// has not been opened.
//
// The caller must ensure that the /L/'s stack has at least 2 free slots.
LS_INHEADER void ls_self_pipe_push_luafunc(int fds[2], lua_State *L)
{
    lua_pushlightuserdata(L, fds);
    lua_pushcclosure(L, ls_self_pipe_lfunc, 1);
}

#endif
