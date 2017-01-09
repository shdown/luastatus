#ifndef check_lua_call_h_
#define check_lua_call_h_

#include <lua.h>
#include <stdbool.h>

bool
check_lua_call(lua_State *L, int retval);

#endif
