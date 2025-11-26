/*
 * Copyright (C) 2025  luastatus developers
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

#include "runshell.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <spawn.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <lua.h>
#include <lauxlib.h>

#if LUA_VERSION_NUM >= 504
#   define my_pushfail(L_) luaL_pushfail(L_)
#else
#   define my_pushfail(L_) lua_pushnil(L_)
#endif

#define CANNOT_FAIL(Expr_) \
    do { \
        if ((Expr_) < 0) { \
            perror("librunshell: " #Expr_ " failed"); \
            abort(); \
        } \
    } while (0)

#define CANNOT_FAIL_PTH(Expr_) \
    do { \
        if ((errno = (Expr_)) != 0) { \
            perror("librunshell: " #Expr_ " failed"); \
            abort(); \
        } \
    } while (0)

static __thread char errbuf[512];

#if _POSIX_C_SOURCE < 200112L || defined(_GNU_SOURCE)
#   error "Unsupported feature test macros."
#endif

static __thread char errbuf[512];

static const char *my_strerror(int errnum)
{
    // We introduce an /int/ variable in order to get a compilation warning if /strerror_r()/ is
    // still GNU-specific and returns a pointer to char.
    int r = strerror_r(errnum, errbuf, sizeof(errbuf));
    return r == 0 ? errbuf : "unknown error or truncated error message";
}

extern char **environ;

int runshell(const char *cmd)
{
    if (!cmd) {
        fputs("librunshell: passed cmd == NULL (this is not supported)\n", stderr);
        abort();
    }

    sigset_t ss_new;
    sigset_t ss_old;
    CANNOT_FAIL(sigemptyset(&ss_new));
    CANNOT_FAIL(sigaddset(&ss_new, SIGCHLD));
    CANNOT_FAIL(sigprocmask(SIG_BLOCK, &ss_new, &ss_old));

    posix_spawnattr_t attr;
    CANNOT_FAIL_PTH(posix_spawnattr_init(&attr));
    CANNOT_FAIL_PTH(posix_spawnattr_setsigmask(&attr, &ss_old));
    CANNOT_FAIL_PTH(posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSIGMASK));

    char *const argv[] = {
        (char *) "sh",
        (char *) "-c",
        (char *) cmd,
        NULL,
    };
    pid_t pid;
    int rc = posix_spawn(
        /*pid=*/ &pid,
        /*path=*/ "/bin/sh",
        /*file_actions=*/ NULL,
        /*attrp=*/ &attr,
        /*argv=*/ argv,
        /*envp=*/ environ);

    CANNOT_FAIL_PTH(posix_spawnattr_destroy(&attr));

    int ret;
    int saved_errno;
    if (rc == 0) {
        saved_errno = 0;
        while (waitpid(pid, &ret, 0) < 0) {
            if (errno != EINTR) {
                ret = -1;
                saved_errno = errno;
                break;
            }
        }
    } else {
        ret = -1;
        saved_errno = rc;
    }

    CANNOT_FAIL(sigprocmask(SIG_SETMASK, &ss_old, NULL));

    errno = saved_errno;
    return ret;
}

int runshell_l_os_execute_lua51ver(lua_State *L)
{
    const char *cmd = luaL_optstring(L, 1, NULL);
    // L: ?
    if (!cmd) {
        lua_pushinteger(L, 1); // L: ? 1
        return 1;
    }
    lua_pushinteger(L, runshell(cmd)); // L: ? code
    return 1;
}

int runshell_l_os_execute(lua_State *L)
{
    const char *cmd = luaL_optstring(L, 1, NULL);
    // L: ?
    if (!cmd) {
        lua_pushboolean(L, 1); // L: ? true
        return 1;
    }
    int rc = runshell(cmd);
    if (rc < 0) {
        int saved_errno = errno;
        my_pushfail(L); // L: ? fail
        lua_pushstring(L, my_strerror(saved_errno)); // L: ? fail err_msg
        lua_pushinteger(L, saved_errno); // L: ? fail err_msg errno
        return 3;
    }
    int code;
    bool normal_exit = true;
    if (WIFEXITED(rc)) {
        code = WEXITSTATUS(rc);
    } else if (WIFSIGNALED(rc)) {
        code = WTERMSIG(rc);
        normal_exit = false;
    } else {
        code = rc;
    }
    if (normal_exit && code == 0) {
        lua_pushboolean(L, 1); // L: ? true
    } else {
        my_pushfail(L); // L: ? fail
    }
    lua_pushstring(L, normal_exit ? "exit" : "signal"); // L: ? is_ok what
    lua_pushinteger(L, code); // L: ? is_ok what code
    return 3;
}
