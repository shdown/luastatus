#include "zoo_checkudata.h"
#include <lua.h>
#include <lauxlib.h>
#include "libls/ls_panic.h"

void *zoo_checkudata(lua_State *L, int pos, const char *tname, const char *what)
{
    // L: ?
    void *ud = lua_touserdata(L, pos);
    if (!ud) {
        goto error;
    }
    if (!lua_getmetatable(L, pos)) {
        goto error;
    }
    // L: ? actual_mt
    lua_getfield(L, LUA_REGISTRYINDEX, tname);
    // L: ? actual_mt expected_mt
    if (!lua_rawequal(L, -1, -2)) {
        goto error;
    }
    lua_pop(L, 2); // L: ?
    return ud;
error:
    (void) luaL_error(L, "%s: is not a '%s' userdata value", what, tname);
    LS_MUST_BE_UNREACHABLE();
}
