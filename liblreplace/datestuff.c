/*
 * Copyright (C) 2026  luastatus developers
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

#include "datestuff.h"
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <lua.h>
#include <lauxlib.h>

#if LUA_VERSION_NUM < 501
# error "Unsupported LUA_VERSION_NUM."
#endif

#if LUA_VERSION_NUM == 501

// Almost literally copy-pasted from Lua 5.1 implementation, modulo code style and usage
// of 'gmtime_r()'/'localtime_r()' vs 'gmtime()'/'localtime()'.

static void setfield(lua_State *L, const char *key, int value)
{
    lua_pushinteger(L, value);
    lua_setfield(L, -2, key);
}

static void setboolfield(lua_State *L, const char *key, int value)
{
    if (value < 0) { // undefined?
        return; // does not set field
    }
    lua_pushboolean(L, value);
    lua_setfield(L, -2, key);
}

static int l_os_date(lua_State *L) /*__THROWABLE__*/
{
    struct tm storage;

    const char *s = luaL_optstring(L, 1, "%c");
    time_t t = luaL_opt(L, (time_t) luaL_checknumber, 2, time(NULL));
    struct tm *stm;
    if (*s == '!') { // UTC?
        stm = gmtime_r(&t, &storage);
        s++; // skip `!'
    } else {
        stm = localtime_r(&t, &storage);
    }
    if (stm == NULL) { // invalid date?
        lua_pushnil(L);
    } else if (strcmp(s, "*t") == 0) {
        lua_createtable(L, 0, 9); // 9 = number of fields
        setfield(L, "sec", stm->tm_sec);
        setfield(L, "min", stm->tm_min);
        setfield(L, "hour", stm->tm_hour);
        setfield(L, "day", stm->tm_mday);
        setfield(L, "month", stm->tm_mon + 1);
        setfield(L, "year", stm->tm_year + 1900);
        setfield(L, "wday", stm->tm_wday + 1);
        setfield(L, "yday", stm->tm_yday + 1);
        setboolfield(L, "isdst", stm->tm_isdst);
    } else {
        char cc[3];
        luaL_Buffer b;
        cc[0] = '%';
        cc[2] = '\0';
        luaL_buffinit(L, &b);
        for (; *s; s++) {
            if (*s != '%' || *(s + 1) == '\0') { // no conversion specifier?
                luaL_addchar(&b, *s);
            } else {
                size_t reslen;
                char buff[200]; // should be big enough for any conversion result
                cc[1] = *(++s);
                reslen = strftime(buff, sizeof(buff), cc, stm);
                luaL_addlstring(&b, buff, reslen);
            }
        }
        luaL_pushresult(&b);
    }
    return 1;
}

void liblreplace_datestuff_inject(lua_State *L)
{
    // See 'DOCS/c_notes/lua_cfuncs.md' in the root of the repo for what __OK__ means.

    // L: ?
    lua_getglobal(L, "os"); // L: ? os
    /*__OK__*/ lua_pushcfunction(L, l_os_date); // L: ? os func
    lua_setfield(L, -2, "date"); // L: ? os
    lua_pop(L, 1); // L: ?
}

#else
void liblreplace_datestuff_inject(lua_State *L)
{
    (void) L;
}
#endif
