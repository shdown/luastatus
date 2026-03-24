#include "load_lualib.h"
#include <lua.h>
#include <lauxlib.h>
#include <string.h>
#include "include/sayf_macros.h"
#include "include/plugin_data_v1.h"

static void report_and_pop_error(LuastatusPluginData *pd, lua_State *L, int rc)
{
    const char *msg = lua_tostring(L, -1);
    if (!msg) {
        msg = "(error object cannot be converted to string)";
    }
    LS_ERRF(pd, "cannot load lualib.lua: [%d] %s", rc, msg);

    lua_pop(L, 1); // L: ?
}

bool load_lualib(LuastatusPluginData *pd, lua_State *L)
{
    static const char *LUA_SOURCE =
#include "lualib.generated.inc"
    ;
    int rc;

    // L: ? table
    rc = luaL_loadbuffer(L, LUA_SOURCE, strlen(LUA_SOURCE), "<bundled lualib.lua>");
    if (rc != 0) {
        // L: ? table error
        report_and_pop_error(pd, L, rc);
        // L: ? table
        return false;
    }
    // L: ? table chunk
    lua_pushvalue(L, -2); // L: ? table chunk table

    rc = lua_pcall(L, 1, 0, 0);
    if (rc != 0) {
        // L: ? table error
        report_and_pop_error(pd, L, rc);
        // L: ? table
        return false;
    }
    // L: ? table

    return true;
}
