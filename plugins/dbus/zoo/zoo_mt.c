#include "zoo_mt.h"

#include <lua.h>
#include <lauxlib.h>

void zoo_mt_begin(lua_State *L, const char *mt_name)
{
    // L: ?
    luaL_newmetatable(L, mt_name); // L: ? mt

    lua_pushvalue(L, -1); // L: ? mt mt
    lua_setfield(L, -2, "__index"); // L: ? mt
}

void zoo_mt_add_method(lua_State *L, const char *name, lua_CFunction f)
{
    lua_pushcfunction(L, f); // L: ? mt f
    lua_setfield(L, -2, name); // L: ? mt
}

void zoo_mt_end(lua_State *L)
{
    // L: ? mt
    lua_pop(L, 1); // L: ?
}
