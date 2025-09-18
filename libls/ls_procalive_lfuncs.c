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

#include "ls_procalive_lfuncs.h"
#include <lua.h>
#include <lauxlib.h>
#include <signal.h>
#include <stdbool.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ls_tls_ebuf.h"

int ls_procalive_lfunc_access(lua_State *L)
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
            lua_pushstring(L, ls_tls_strerror(saved_errno));
            // L: false str
        }
    }
    return 2;
}

int ls_procalive_lfunc_stat(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);

    struct stat sb;
    if (stat(path, &sb) < 0) {
        int saved_errno = errno;
        lua_pushnil(L); // L: nil
        lua_pushstring(L, ls_tls_strerror(saved_errno)); // L: nil err
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

bool push_glob_t(lua_State *L, glob_t *g)
{
#if defined(LUA_MAXINTEGER)
# define CMP_AGAINST LUA_MAXINTEGER
#else
# define CMP_AGAINST INT32_MAX
#endif

#if CMP_AGAINST < SIZE_MAX
    if (g->gl_pathc > (size_t) CMP_AGAINST) {
        return false;
    }
#endif

#undef CMP_AGAINST

    lua_createtable(L, g->gl_pathc, 0);
    // L: array
    for (size_t i = 0; i < g->gl_pathc; ++i) {
        lua_pushstring(L, g->gl_pathv[i]); // L: array str
        lua_rawseti(L, -2, i + 1); // L: array
    }
    return true;
}

int ls_procalive_lfunc_glob(lua_State *L)
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
    lua_pushstring(L, ls_tls_strerror(err_num)); // L: nil err
    return 2;
}

int ls_procalive_lfunc_is_process_alive(lua_State *L)
{
    double pid_d = luaL_checknumber(L, 1);
    if (!(pid_d >= 1)) {
        return luaL_argerror(L, 1, "must be >= 1");
    }
    if (pid_d >= UINT_MAX) {
        return luaL_argerror(L, 1, "too large");
    }

    pid_t pid = (pid_t) (unsigned) pid_d;
    if (pid <= 0) {
        return luaL_argerror(L, 1, "somehow out of range of pid_t");
    }

    bool res = (kill(pid, 0) >= 0 || errno == EPERM);

    lua_pushboolean(L, res); // L: res
    return 1;
}

void ls_procalive_lfuncs_register_all(lua_State *L)
{
    // L: table

    lua_pushcfunction(L, ls_procalive_lfunc_access); // L: table func
    lua_setfield(L, -2, "access"); // L: table

    lua_pushcfunction(L, ls_procalive_lfunc_stat); // L: table func
    lua_setfield(L, -2, "stat"); // L: table

    lua_pushcfunction(L, ls_procalive_lfunc_glob); // L: table func
    lua_setfield(L, -2, "glob"); // L: table

    lua_pushcfunction(L, ls_procalive_lfunc_is_process_alive); // L: table func
    lua_setfield(L, -2, "is_process_alive"); // L: table
}
