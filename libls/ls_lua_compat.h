#pragma once

#include <lua.h>
#include <lauxlib.h>
#include <stdbool.h>
#include <stddef.h>
#include "ls_compdep.h"

#if LUA_VERSION_NUM >= 504
#   define ls_lua_pushfail(L_) luaL_pushfail(L_)
#else
#   define ls_lua_pushfail(L_) lua_pushnil(L_)
#endif

LS_INHEADER bool ls_lua_is_lua51(lua_State *L)
{
#if LUA_VERSION_NUM >= 502
    (void) L;
    return false;
#else
    // LuaJIT, when compiled with -DLUAJIT_ENABLE_LUA52COMPAT, still defines
    // LUA_VERSION_NUM to 501, but syntax parser, library functions and some
    // aspect of language work as if it was Lua 5.2.

    // L: ?
    lua_getglobal(L, "rawlen"); // L: ? rawlen
    bool ret = lua_isnil(L, -1);
    lua_pop(L, 1); // L: ?
    return ret;
#endif
}

LS_INHEADER size_t ls_lua_array_len(lua_State *L, int pos)
{
#if LUA_VERSION_NUM <= 501
    return lua_objlen(L, pos);
#else
    return lua_rawlen(L, pos);
#endif
}
