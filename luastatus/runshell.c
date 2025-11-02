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
#include <stddef.h>
#include <stdbool.h>
#include <signal.h>
#include <spawn.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <lua.h>
#include <lauxlib.h>
#include "libls/ls_panic.h"
#include "libls/ls_lua_compat.h"
#include "libls/ls_tls_ebuf.h"

#define CANNOT_FAIL(Expr_) \
    do { \
        if ((Expr_) < 0) { \
            LS_PANIC_WITH_ERRNUM("runshell: " #Expr_ " failed", errno); \
        } \
    } while (0)

extern char **environ;

int runshell(const char *cmd)
{
    sigset_t ss_new;
    sigset_t ss_old;
    CANNOT_FAIL(sigemptyset(&ss_new));
    CANNOT_FAIL(sigaddset(&ss_new, SIGCHLD));
    CANNOT_FAIL(sigprocmask(SIG_BLOCK, &ss_new, &ss_old));

    posix_spawnattr_t attr;
    LS_PTH_CHECK(posix_spawnattr_init(&attr));
    LS_PTH_CHECK(posix_spawnattr_setsigmask(&attr, &ss_old));
    LS_PTH_CHECK(posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSIGMASK));

    char *const argv[] = {
        (char *) "sh",
        (char *) "-c",
        (char *) cmd,
    };
    pid_t pid;
    int rc = posix_spawn(
        /*pid=*/ &pid,
        /*path=*/ "/bin/sh",
        /*file_actions=*/ NULL,
        /*attrp=*/ &attr,
        /*argv=*/ argv,
        /*envp=*/ environ);

    LS_PTH_CHECK(posix_spawnattr_destroy(&attr));

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

int l_os_execute_lua51ver(lua_State *L)
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

int l_os_execute(lua_State *L)
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
        ls_lua_pushfail(L); // L: ? fail
        lua_pushstring(L, ls_tls_strerror(saved_errno)); // L: ? fail err_msg
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
        ls_lua_pushfail(L); // L: ? fail
    }
    lua_pushstring(L, normal_exit ? "exit" : "signal"); // L: ? is_ok what
    lua_pushinteger(L, code); // L: ? is_ok what code
    return 3;
}
