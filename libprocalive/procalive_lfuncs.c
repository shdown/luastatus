/*
 * Copyright (C) 2015-2025  luastatus developers
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

#include "procalive_lfuncs.h"
#include <lua.h>
#include <lauxlib.h>
#include <luaconf.h>
#include <signal.h>
#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>

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

#ifdef LUA_MAXINTEGER
# define MAXI \
    (LUA_MAXINTEGER > (SIZE_MAX - 1) ? (SIZE_MAX - 1) : LUA_MAXINTEGER)
#else
# define MAXI INT_MAX
#endif

int procalive_lfunc_access(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);

    if (access(path, F_OK) >= 0) {
        lua_pushboolean(L, 1);
        // L: true
        lua_pushnil(L);
        // L: true nil
    } else {
        int saved_errno = errno;
        lua_pushboolean(L, 0);
        // L: false
        if (saved_errno == ENOENT) {
            lua_pushnil(L);
            // L: false nil
        } else {
            lua_pushstring(L, my_strerror(saved_errno));
            // L: false str
        }
    }
    return 2;
}

int procalive_lfunc_stat(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);

    struct stat sb;
    if (stat(path, &sb) < 0) {
        int saved_errno = errno;
        lua_pushnil(L); // L: nil
        lua_pushstring(L, my_strerror(saved_errno)); // L: nil err
        return 2;
    }

    mode_t M = sb.st_mode;
    const char *M_str;

    if (S_ISREG(M)) {
        M_str = "regular";

    } else if (S_ISDIR(M)) {
        M_str = "dir";

    } else if (S_ISCHR(M)) {
        M_str = "chardev";

    } else if (S_ISBLK(M)) {
        M_str = "blockdev";

    } else if (S_ISFIFO(M)) {
        M_str = "fifo";

    } else if (S_ISLNK(M)) {
        M_str = "symlink";

    } else if (S_ISSOCK(M)) {
        M_str = "socket";

    } else {
        M_str = "other";
    }

    lua_pushstring(L, M_str); // L: res
    lua_pushnil(L); // L: res nil
    return 2;
}

static inline int get_lua_num_prealloc(size_t n)
{
    return n < (size_t) INT_MAX ? (int) n : INT_MAX;
}

static bool push_glob_t(lua_State *L, glob_t *g)
{
    size_t n = g->gl_pathc;
    if (n > (size_t) MAXI) {
        return false;
    }
    lua_createtable(L, get_lua_num_prealloc(n), 0); // L: array
    for (size_t i = 0; i < n; ++i) {
        lua_pushstring(L, g->gl_pathv[i]); // L: array str
        lua_rawseti(L, -2, i + 1); // L: array
    }
    return true;
}

int procalive_lfunc_glob(lua_State *L)
{
    const char *pattern = luaL_checkstring(L, 1);

    glob_t g = {0};
    int rc = glob(pattern, GLOB_MARK | GLOB_NOSORT, NULL, &g);
    int err_num;

    switch (rc) {
    case 0:
        if (!push_glob_t(L, &g)) {
            err_num = EOVERFLOW;
            goto fail;
        }
        // L: res
        goto ok;

    case GLOB_NOMATCH:
        lua_newtable(L); // L: res
        goto ok;

    case GLOB_ABORTED:
        err_num = EIO;
        goto fail;

    case GLOB_NOSPACE:
        err_num = ENOMEM;
        goto fail;

    default:
        err_num = EINVAL;
        goto fail;
    }

ok:
    globfree(&g);
    // L: res
    lua_pushnil(L); // L: res nil
    return 2;

fail:
    globfree(&g);
    lua_pushnil(L); // L: nil
    lua_pushstring(L, my_strerror(err_num)); // L: nil err
    return 2;
}

static pid_t parse_pid(const char *s, const char **out_err_msg)
{
    char *endptr;
    errno = 0;
    intmax_t mres = strtoimax(s, &endptr, 10);

    if (errno != 0 || *s == '\0' || *endptr != '\0') {
        *out_err_msg = "cannot parse into intmax_t";
        return -1;
    }

    pid_t res = mres;
    if (res <= 0 || res != mres) {
        *out_err_msg = "outside of valid range for pid_t";
        return -1;
    }
    return res;
}

int procalive_lfunc_is_process_alive(lua_State *L)
{
    luaL_checkany(L, 1);

    pid_t pid;

    if (lua_isstring(L, 1)) {
        const char *err_msg;
        pid = parse_pid(lua_tostring(L, 1), &err_msg);
        if (pid <= 0) {
            return luaL_argerror(L, 1, err_msg);
        }
    } else if (lua_isnumber(L, 1)) {
        double d = lua_tonumber(L, 1);
        if (!isgreaterequal(d, 1.0)) {
            return luaL_argerror(L, 1, "must be >= 1");
        }
        if (d > INT_MAX) {
            return luaL_argerror(L, 1, "too large");
        }
        int i = (int) d;
        pid = (pid_t) i;
        if (pid <= 0 || pid != i) {
            return luaL_argerror(L, 1, "outside of valid range for pid_t");
        }
    } else {
        return luaL_argerror(L, 1, "neither number nor string");
    }

    bool res = (kill(pid, 0) >= 0 || errno == EPERM);

    lua_pushboolean(L, res); // L: res
    return 1;
}

void procalive_lfuncs_register_all(lua_State *L)
{
    // L: table

    lua_pushcfunction(L, procalive_lfunc_access); // L: table func
    lua_setfield(L, -2, "access"); // L: table

    lua_pushcfunction(L, procalive_lfunc_stat); // L: table func
    lua_setfield(L, -2, "stat"); // L: table

    lua_pushcfunction(L, procalive_lfunc_glob); // L: table func
    lua_setfield(L, -2, "glob"); // L: table

    lua_pushcfunction(L, procalive_lfunc_is_process_alive); // L: table func
    lua_setfield(L, -2, "is_process_alive"); // L: table
}
