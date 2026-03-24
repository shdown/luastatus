#include "zoo_call_prot.h"
#include <stdbool.h>
#include <lua.h>

bool zoo_call_prot(lua_State *L, int nargs, int nresults, lua_CFunction f, void *f_ud)
{
    // L: ? args
    lua_pushlightuserdata(L, f_ud); // L: ? args ud
    lua_pushcclosure(L, f, 1); // L: ? args func
    lua_insert(L, -(nargs + 1)); // L: ? func args
    return lua_pcall(L, nargs, nresults, 0) == 0;
}
