#ifndef luastatus_include_plugin_utils_h_
#define luastatus_include_plugin_utils_h_

#include <stddef.h>
#include <stdbool.h>
#include <lua.h>
#include <lauxlib.h>
#include "libls/lua_utils.h"
#include "include/loglevel.h"

#ifndef PU_L_NAME
#define PU_L_NAME L
#endif

#ifndef PU_PD_NAME
#define PU_PD_NAME pd
#endif

#ifndef PU_ON_ERROR
#define PU_ON_ERROR goto error;
#endif

//------------------------------------------------------------------------------

#define PU_CHECK_TYPE_AT(StackIndex_, StringRepr_, LuaType_) \
    do { \
        if (lua_type(PU_L_NAME, StackIndex_) != LuaType_) { \
            PU_PD_NAME->logf(PU_PD_NAME->userdata, LUASTATUS_FATAL, "%s: expected %s, found %s", \
                             StringRepr_, lua_typename(PU_L_NAME, LuaType_), \
                             luaL_typename(PU_L_NAME, StackIndex_)); \
            PU_ON_ERROR \
        } \
    } while (0)

//------------------------------------------------------------------------------

#define PU_VISIT_STR_AT(StackIndex_, StringRepr_, Var_, ...) \
    do { \
        PU_CHECK_TYPE_AT(StackIndex_, StringRepr_, LUA_TSTRING); \
        const char *Var_ = lua_tostring(PU_L_NAME, StackIndex_); \
        __VA_ARGS__ \
    } while (0)

#define PU_VISIT_LSTR_AT(StackIndex_, StringRepr_, SVar_, NVar_, ...) \
    do { \
        PU_CHECK_TYPE_AT(StackIndex_, StringRepr_, LUA_TSTRING); \
        size_t NVar_; \
        const char *SVar_ = lua_tolstring(PU_L_NAME, StackIndex_, &NVar_); \
        __VA_ARGS__ \
    } while (0)

#define PU_VISIT_NUM_AT(StackIndex_, StringRepr_, Var_, ...) \
    do { \
        PU_CHECK_TYPE_AT(StackIndex_, StringRepr_, LUA_TNUMBER); \
        lua_Number Var_ = lua_tonumber(PU_L_NAME, StackIndex_); \
        __VA_ARGS__ \
    } while (0)

#define PU_VISIT_BOOL_AT(StackIndex_, StringRepr_, Var_, ...) \
    do { \
        PU_CHECK_TYPE_AT(StackIndex_, StringRepr_, LUA_TBOOLEAN); \
        bool Var_ = lua_toboolean(PU_L_NAME, StackIndex_); \
        __VA_ARGS__ \
    } while (0)

// May be useful when you want not to just traverse a table. Dunno.
#define PU_VISIT_TABLE_AT(StackIndex_, StringRepr_, IndexVar_, ...) \
    do { \
        PU_CHECK_TYPE_AT(StackIndex_, StringRepr_, LUA_TTABLE); \
        int IndexVar_ = (StackIndex_); \
        __VA_ARGS__ \
    } while (0)

#define PU_TRAVERSE_TABLE_AT(StackIndex_, StringRepr_, ...) \
    do { \
        PU_CHECK_TYPE_AT(StackIndex_, StringRepr_, LUA_TTABLE); \
        LS_LUA_TRAVERSE(PU_L_NAME, StackIndex_) { \
            __VA_ARGS__ \
        } \
    } while (0)

//------------------------------------------------------------------------------

// do not use directly.
#define PU__EXPAND_AT_KEY(Key_, AtMacro_, ...) \
    do { \
        ls_lua_rawgetf(PU_L_NAME, Key_); \
        AtMacro_(-1, Key_, __VA_ARGS__); \
        lua_pop(PU_L_NAME, 1); \
    } while (0)

// do not use directly.
#define PU__MAYBE_EXPAND_AT_KEY(Key_, AtMacro_, ...) \
    do { \
        ls_lua_rawgetf(PU_L_NAME, Key_); \
        if (!lua_isnil(PU_L_NAME, -1)) { \
            AtMacro_(-1, Key_, __VA_ARGS__); \
        } \
        lua_pop(PU_L_NAME, 1); \
    } while (0)

//------------------------------------------------------------------------------

#define PU_MAYBE_VISIT_STR(Key_, Var_, ...) \
    PU__MAYBE_EXPAND_AT_KEY(Key_, PU_VISIT_STR_AT, Var_, __VA_ARGS__)

#define PU_MAYBE_VISIT_LSTR(Key_, SVar_, NVar_, ...) \
    PU__MAYBE_EXPAND_AT_KEY(Key_, PU_VISIT_LSTR_AT, SVar_, NVar_, __VA_ARGS__)

#define PU_MAYBE_VISIT_NUM(Key_, Var_, ...) \
    PU__MAYBE_EXPAND_AT_KEY(Key_, PU_VISIT_NUM_AT, Var_, __VA_ARGS__)

#define PU_MAYBE_VISIT_BOOL(Key_, Var_, ...) \
    PU__MAYBE_EXPAND_AT_KEY(Key_, PU_VISIT_BOOL_AT, Var_, __VA_ARGS__)

#define PU_MAYBE_VISIT_TABLE(Key_, IndexVar_, ...) \
    PU__MAYBE_EXPAND_AT_KEY(Key_, PU_VISIT_TABLE_AT, IndexVar_, __VA_ARGS__)

#define PU_MAYBE_TRAVERSE_TABLE(Key_, ...) \
    PU__MAYBE_EXPAND_AT_KEY(Key_, PU_TRAVERSE_TABLE_AT, __VA_ARGS__)

//------------------------------------------------------------------------------

#define PU_VISIT_STR(Key_, Var_, ...) \
    PU__EXPAND_AT_KEY(Key_, PU_VISIT_STR_AT, Var_, __VA_ARGS__)

#define PU_VISIT_LSTR(Key_, SVar_, NVar_, ...) \
    PU__EXPAND_AT_KEY(Key_, PU_VISIT_LSTR_AT, SVar_, NVar_, __VA_ARGS__)

#define PU_VISIT_NUM(Key_, Var_, ...) \
    PU__EXPAND_AT_KEY(Key_, PU_VISIT_NUM_AT, Var_, __VA_ARGS__)

#define PU_VISIT_BOOL(Key_, Var_, ...) \
    PU__EXPAND_AT_KEY(Key_, PU_VISIT_BOOL_AT, Var_, __VA_ARGS__)

#define PU_TRAVERSE_TABLE(Key_, ...) \
    PU__EXPAND_AT_KEY(Key_, PU_TRAVERSE_TABLE_AT, __VA_ARGS__)

#define PU_VISIT_TABLE(Key_, IndexVar_, ...) \
    PU__EXPAND_AT_KEY(Key_, PU_VISIT_TABLE_AT, IndexVar_, __VA_ARGS__)

#endif
