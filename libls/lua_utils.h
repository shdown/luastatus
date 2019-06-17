#ifndef ls_lua_utils_h_
#define ls_lua_utils_h_

#include <lua.h>

// Traverse a table.
// Before using /break/ in this cycle, call /LS_LUA_BREAK(L_)/.
#define LS_LUA_TRAVERSE(L_, StackIndex_) \
    for (lua_pushnil(L_); \
         /* if /StackIndex_/ is relative to the top, decrease it by one because the previous key
          * will be pushed onto the stack each time lua_next() is called. */ \
         lua_next(L_, (StackIndex_) - ((StackIndex_) < 0 ? 1 : 0)); \
         lua_pop(L_, 1))

// Stack index of the key when in a /LS_LUA_TRAVERSE/ cycle.
#define LS_LUA_KEY   (-2)
// Stack index of the value when in a /LS_LUA_TRAVERSE/ cycle.
#define LS_LUA_VALUE (-1)
// Call before using /break/ to leave a /LS_LUA_TRAVERSE/ cycle.
#define LS_LUA_BREAK(L_) lua_pop(L_, 2)

#endif
