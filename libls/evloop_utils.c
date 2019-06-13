#include "evloop_utils.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <lauxlib.h>
#include <sys/stat.h>

#include "time_utils.h"
#include "panic.h"
#include "osdep.h"
#include "io_utils.h"
#include "sig_utils.h"

void
ls_pushed_timeout_init(LSPushedTimeout *p)
{
    p->value = ls_timespec_invalid;
    LS_PTH_CHECK(pthread_spin_init(&p->lock, PTHREAD_PROCESS_PRIVATE));
}

struct timespec
ls_pushed_timeout_fetch(LSPushedTimeout *p, struct timespec alt)
{
    pthread_spin_lock(&p->lock);

    struct timespec r;
    if (ls_timespec_is_invalid(p->value)) {
        r = alt;
    } else {
        r = p->value;
        p->value = ls_timespec_invalid;
    }

    pthread_spin_unlock(&p->lock);
    return r;
}

static
int
l_push_timeout(lua_State *L)
{
    const lua_Number arg = luaL_checknumber(L, 1);
    const struct timespec value = ls_timespec_from_seconds(arg);
    if (ls_timespec_is_invalid(value)) {
        return luaL_error(L, "invalid timeout");
    }

    LSPushedTimeout *p = lua_touserdata(L, lua_upvalueindex(1));

    pthread_spin_lock(&p->lock);
    p->value = value;
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
    if (fd < 0) {
        return luaL_error(L, "self-pipe has not been opened");
    }

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

void
ls_wakeup_fifo_init(LSWakeupFifo *w, const char *fifo, sigset_t *sigmask)
{
    w->fifo = fifo;
    if (sigmask) {
        w->sigmask = *sigmask;
    } else {
        ls_xsigfillset(&w->sigmask);
    }
    w->fifo_fd = -1;
    FD_ZERO(&w->fds);
}

int
ls_wakeup_fifo_open(LSWakeupFifo *w)
{
    int err;
    if (w->fifo_fd < 0 && w->fifo) {
        w->fifo_fd = open(w->fifo, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
        if (w->fifo_fd < 0) {
            return -1;
        }

        struct stat sb;
        if (fstat(w->fifo_fd, &sb) < 0) {
            err = errno;
            goto error;
        } else if (!S_ISFIFO(sb.st_mode)) {
            err = -EINVAL;
            goto error;
        }

        if (w->fifo_fd >= FD_SETSIZE) {
            err = EMFILE;
            goto error;
        }
    }
    return 0;

error:
    close(w->fifo_fd);
    w->fifo_fd = -1;
    errno = err;
    return -1;
}

int
ls_wakeup_fifo_wait(LSWakeupFifo *w, struct timespec timeout)
{
    if (w->fifo_fd >= 0) {
        FD_SET(w->fifo_fd, &w->fds);
    }

    const int r = pselect(
        w->fifo_fd >= 0 ? w->fifo_fd + 1 : 0,
        &w->fds, NULL, NULL,
        ls_timespec_is_invalid(timeout) ? NULL : &timeout,
        &w->sigmask);

    if (r < 0) {
        int saved_errno = errno;
        if (w->fifo_fd >= 0) {
            FD_CLR(w->fifo_fd, &w->fds);
        }
        close(w->fifo_fd);
        w->fifo_fd = -1;
        errno = saved_errno;
        return -1;

    } else if (r == 0) {
        return 0;

    } else {
        FD_CLR(w->fifo_fd, &w->fds);
        close(w->fifo_fd);
        w->fifo_fd = -1;
        return 1;
    }
}

void
ls_wakeup_fifo_destroy(LSWakeupFifo *w)
{
    close(w->fifo_fd);
}
