#include "check_lua_call.h"

#include <lua.h>
#include <stdbool.h>
#include <lauxlib.h>
#include "include/common.h"
#include "log.h"

bool
check_lua_call(lua_State *L, int retval)
{
    const char *prefix;
    switch (retval) {
    case 0:
        return true;
    case LUA_ERRRUN:
    case LUA_ERRSYNTAX:
        prefix = "(lua) ";
        break;
    case LUA_ERRMEM:
        prefix = "(lua) out of memory: ";
        break;
    case LUA_ERRERR:
        prefix = "(lua) error while running error handler: ";
        break;
    case LUA_ERRFILE:
        // Lua itself prepends an error message with "cannot open <filename>: "
        prefix = "(lua) ";
        break;
#ifdef LUA_ERRGCMM
    // first introduced in Lua 5.2
    case LUA_ERRGCMM:
        prefix = "(lua) error while running __gc metamethod: ";
        break;
#endif
    default:
        prefix = "unknown Lua error code (please report!), message is: ";
    }
    const char *msg = lua_tostring(L, -1);
    if (!msg) {
        msg = "(error object can't be converted to string)";
    }
    sayf(LUASTATUS_ERR, "%s%s", prefix, msg);
    lua_pop(L, 1);
    return false;
}
