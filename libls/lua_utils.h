#ifndef ls_lua_utils_h_
#define ls_lua_utils_h_

#include <lua.h>

#include "compdep.h"

// Traverse a table.
// Before using /break/ in this cycle, call /LS_LUA_BREAK(L_)/.
#define LS_LUA_TRAVERSE(L_, StackIndex_) \
    for (lua_pushnil(L_); \
         /* if /StackIndex_/ is relative to the top, decrease it by one because the previous key
          * will be pushed onto the stack each time lua_next() is called. */ \
         lua_next(L_, (StackIndex_) < 0 ? (StackIndex_) - 1 : (StackIndex_)); \
         lua_pop(L_, 1))

// Stack index of the key when in a /LS_LUA_TRAVERSE/ cycle.
#define LS_LUA_KEY   (-2)
// Stack index of the value when in a /LS_LUA_TRAVERSE/ cycle.
#define LS_LUA_VALUE (-1)
// Call before using /break/ to leave a /LS_LUA_TRAVERSE/ cycle.
#define LS_LUA_BREAK(L_) lua_pop(L_, 2)

// The behaviour is same as calling /lua_getfield(L, -1, key)/, except that it does not invoke
// metamethods.
LS_INHEADER
void
ls_lua_rawgetf(lua_State *L, const char *key)
{
    lua_pushstring(L, key);
    lua_rawget(L, -2);
}

// The behaviour is same as calling /lua_setfield(L, -2, key)/, except that it does not invoke
// metamethods.
LS_INHEADER
void
ls_lua_rawsetf(lua_State *L, const char *key)
{
    lua_pushstring(L, key);
    lua_insert(L, -2);
    lua_rawset(L, -3);
}

// Pushes the global table onto the stack. The behaviour is same as calling
// /lua_pushglobaltable(L_)/ in Lua >=5.2.
#if LUA_VERSION_NUM >= 502
#   define ls_lua_pushg     lua_pushglobaltable
#else
#   define ls_lua_pushg(L_) lua_pushvalue(L_, LUA_GLOBALSINDEX)
#endif

#endif
