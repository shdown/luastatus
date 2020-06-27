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

#include "evloop_utils.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <lauxlib.h>
#include <signal.h>
#include <sys/stat.h>

#include "time_utils.h"
#include "panic.h"
#include "osdep.h"
#include "io_utils.h"

void
ls_pushed_timeout_init(LSPushedTimeout *p)
{
    p->value = -1;
    LS_PTH_CHECK(pthread_spin_init(&p->lock, PTHREAD_PROCESS_PRIVATE));
}

double
ls_pushed_timeout_fetch(LSPushedTimeout *p, double alt)
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

static
int
l_push_timeout(lua_State *L)
{
    double arg = luaL_checknumber(L, 1);
    if (arg < 0)
        return luaL_error(L, "invalid timeout");

    LSPushedTimeout *p = lua_touserdata(L, lua_upvalueindex(1));

    pthread_spin_lock(&p->lock);
    p->value = arg;
    pthread_spin_unlock(&p->lock);

    return 0;
}

void
ls_pushed_timeout_push_luafunc(LSPushedTimeout *p, lua_State *L)
{
    lua_pushlightuserdata(L, p);
    lua_pushcclosure(L, l_push_timeout, 1);
}

void
ls_pushed_timeout_destroy(LSPushedTimeout *p)
{
    pthread_spin_destroy(&p->lock);
}

int
ls_self_pipe_open(LSSelfPipe *s)
{
    if (ls_cloexec_pipe(s->fds) < 0) {
        s->fds[0] = -1;
        s->fds[1] = -1;
        return -1;
    }
    ls_make_nonblock(s->fds[0]);
    ls_make_nonblock(s->fds[1]);
    return 0;
}

static
int
l_self_pipe_write(lua_State *L)
{
    LSSelfPipe *s = lua_touserdata(L, lua_upvalueindex(1));

    const int fd = s->fds[1];
    if (fd < 0)
        return luaL_error(L, "self-pipe has not been opened");

    ssize_t unused = write(fd, "", 1); // write '\0'
    (void) unused;

    return 0;
}

void
ls_self_pipe_push_luafunc(LSSelfPipe *s, lua_State *L)
{
    lua_pushlightuserdata(L, s);
    lua_pushcclosure(L, l_self_pipe_write, 1);
}

void
ls_self_pipe_close(LSSelfPipe *s)
{
    close(s->fds[0]);
    close(s->fds[1]);
}

int
ls_poll(struct pollfd *fds, nfds_t nfds, double tmo)
{
    int tmo_ms = ls_tmo_to_ms(tmo);

    sigset_t allsigs;
    sigfillset(&allsigs);

    sigset_t origmask;
    sigprocmask(SIG_SETMASK, &allsigs, &origmask);
    int r = poll(fds, nfds, tmo_ms);
    sigprocmask(SIG_SETMASK, &origmask, NULL);

    return r;
}

int
ls_wait_input_on_fd(int fd, double tmo)
{
    struct pollfd x = {.fd = fd, .events = POLLIN};
    return ls_poll(&x, 1, tmo);
}

int
ls_fifo_open(int *fd, const char *fifo)
{
    int saved_errno;

    if (*fd >= 0)
        return 0;

    if (!fifo)
        return 0;

    *fd = open(fifo, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (*fd < 0)
        goto error;

    struct stat sb;
    if (fstat(*fd, &sb) < 0)
        goto error;

    if (!S_ISFIFO(sb.st_mode)) {
        errno = -EINVAL;
        goto error;
    }

    return 0;

error:
    saved_errno = errno;
    close(*fd);
    *fd = -1;
    errno = saved_errno;
    return -1;
}

int
ls_fifo_wait(int *fd, double tmo)
{
    int r = ls_wait_input_on_fd(*fd, tmo);
    if (r > 0) {
        close(*fd);
        *fd = -1;
    }
    return r;
}
