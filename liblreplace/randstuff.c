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

#include "randstuff.h"

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <lua.h>
#include <lauxlib.h>

#if LUA_VERSION_NUM >= 504

void liblreplace_randstuff_inject(lua_State *L)
{
    (void) L;
}

#else

// We use the same PRNG that is used in glibc.

typedef uint32_t PRNG;

enum { PRNG_MAX = 0x7fff };

static inline void prng_init(PRNG *x, int seed)
{
    *x = seed;
}

static inline int prng_next(PRNG *x)
{
    *x = *x * 1103515245 + 12345;
    return (*x >> 16) & 0x7fff;
}

static bool check_is_luajit(lua_State *L)
{
    // L: ?
    lua_getglobal(L, "jit"); // L: ? jit
    bool is_luajit = !lua_isnil(L, -1);
    lua_pop(L, 1); // L: ?
    return is_luajit;
}

#if LUA_VERSION_NUM == 501
static int l_math_random(lua_State *L) /*__THROWABLE__*/
{
    PRNG *prng = lua_touserdata(L, lua_upvalueindex(1));

    lua_Number r = ((lua_Number) prng_next(prng)) / (((uint32_t) PRNG_MAX) + 1);

    int nargs = lua_gettop(L);
    if (nargs == 0) {
        lua_pushnumber(L, r);
    } else if (nargs == 1) {
        // only upper limit
        int u = luaL_checkinteger(L, 1);
        luaL_argcheck(L, 1 <= u, 1, "interval is empty");
        lua_pushnumber(L, floor(r * u) + 1);
    } else if (nargs == 2) {
        // lower and upper limits
        int l = luaL_checkinteger(L, 1);
        int u = luaL_checkinteger(L, 2);
        luaL_argcheck(L, l <= u, 2, "interval is empty");
        lua_pushnumber(L, floor(r * (u - l + 1)) + l);
    } else {
        return luaL_error(L, "wrong number of arguments");
    }
    return 1;
}
#elif LUA_VERSION_NUM == 502
static int l_math_random(lua_State *L) /*__THROWABLE__*/
{
    PRNG *prng = lua_touserdata(L, lua_upvalueindex(1));

    lua_Number r = ((lua_Number) prng_next(prng)) / (((uint32_t) PRNG_MAX) + 1);

    int nargs = lua_gettop(L);
    if (nargs == 0) {
        lua_pushnumber(L, r);
    } else if (nargs == 1) {
        // only upper limit
        lua_Number u = luaL_checknumber(L, 1);
        luaL_argcheck(L, ((lua_Number) 1.0) <= u, 1, "interval is empty");
        lua_pushnumber(L, floor(r * u) + (lua_Number) 1.0);
    } else if (nargs == 2) {
        // lower and upper limits
        lua_Number l = luaL_checknumber(L, 1);
        lua_Number u = luaL_checknumber(L, 2);
        luaL_argcheck(L, l <= u, 2, "interval is empty");
        lua_pushnumber(L, floor(r * (u - l + 1)) + l);
    } else {
        return luaL_error(L, "wrong number of arguments");
    }
    return 1;
}
#elif LUA_VERSION_NUM == 503
static int l_math_random(lua_State *L) /*__THROWABLE__*/
{
    PRNG *prng = lua_touserdata(L, lua_upvalueindex(1));

    double r = ((double) prng_next(prng)) * (1.0 / (((double) PRNG_MAX) + 1.0));

    lua_Integer l;
    lua_Integer u;

    int nargs = lua_gettop(L);
    if (nargs == 0) {
        lua_pushnumber(L, r);
        return 1;
    } else if (nargs == 1) {
        // only upper limit
        l = 1;
        u = luaL_checkinteger(L, 1);
    } else if (nargs == 2) {
        // lower and upper limits
        l = luaL_checkinteger(L, 1);
        u = luaL_checkinteger(L, 2);
    } else {
        return luaL_error(L, "wrong number of arguments");
    }

    luaL_argcheck(L, l <= u, 1, "interval is empty");
    luaL_argcheck(L, l >= 0 || u <= LUA_MAXINTEGER + l, 1, "interval too large");
    r *= ((double) (u - l)) + 1.0;
    lua_pushinteger(L, ((lua_Integer) r) + l);

    return 1;
}
#else
# error "Unsupported LUA_VERSION_NUM."
#endif

static int l_math_randomseed(lua_State *L) /*__THROWABLE__*/
{
    PRNG *prng = lua_touserdata(L, lua_upvalueindex(1));

    int seed = luaL_checkinteger(L, 1);
    prng_init(prng, seed);

    return 0;
}

static void reg(lua_State *L, lua_CFunction f, const char *key)
{
    // See 'DOCS/c_notes/lua_cfuncs.md' in the root of the repo for what __OK__ means.

    // L: ? PRNG math
    lua_pushvalue(L, -2); // L: ? PRNG math PRNG
    /*__OK__*/ lua_pushcclosure(L, f, 1); // L: ? PRNG math func
    lua_setfield(L, -2, key); // L: ? PRNG math
}

#if LUA_VERSION_NUM >= 504
# define my_lua_newuserdata(L, sz) lua_newuserdatauv((L), (sz), 1)
#else
# define my_lua_newuserdata(L, sz) lua_newuserdata((L), (sz))
#endif

void liblreplace_randstuff_inject(lua_State *L)
{
    if (check_is_luajit(L)) {
        return;
    }
    // L: ?

    PRNG *prng = my_lua_newuserdata(L, sizeof(PRNG)); // L: ? PRNG
    prng_init(prng, 1);

    lua_getglobal(L, "math"); // L: ? PRNG math

    reg(L, l_math_random, "random");
    reg(L, l_math_randomseed, "randomseed");

    // L: ? PRNG math
    lua_pop(L, 2); // L: ?
}

#endif
