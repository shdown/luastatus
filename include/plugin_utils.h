#ifndef luastatus_include_plugin_utils_h_
#define luastatus_include_plugin_utils_h_

// A number of macros that simplify marshalling widget options for plugins.

#include <stddef.h>
#include <stdbool.h>
#include <lua.h>
#include <lauxlib.h>

#include "libls/lua_utils.h"

#include "common.h"

// A /lua_State */ object that /PU_*/ macros should operate on.
#ifndef PU_L
#   define PU_L L
#endif

// A /LuastatusPluginData */ object that /PU_*/ macros should log errors to.
#ifndef PU_PD
#   define PU_PD pd
#endif

// A chunk of code that /PU_*/ macros should execute should any error happen.
#ifndef PU_ON_ERROR
#   define PU_ON_ERROR goto error;
#endif

//------------------------------------------------------------------------------

// Checks that the object at the index /StackIndex_/ in /PU_L/'s stack has type /LuaType_/.
// /StringRepr_/ should be a string with description of that object; if the type mismatches, it will
// be logged and /PU_ON_ERROR/ will be executed.
#define PU_CHECK_TYPE_AT(StackIndex_, StringRepr_, LuaType_) \
    do { \
        if (lua_type(PU_L, StackIndex_) != LuaType_) { \
            PU_PD->sayf(PU_PD->userdata, LUASTATUS_LOG_FATAL, "%s: expected %s, found %s", \
                        StringRepr_, \
                        lua_typename(PU_L, LuaType_), \
                        luaL_typename(PU_L, StackIndex_)); \
            PU_ON_ERROR \
        } \
    } while (0)

//------------------------------------------------------------------------------

// Checks that the object at the index /StackIndex_/ in /PU_L/'s stack is a string.
// Creates a /const char */ variable named /Var_/ with the value of that string (as if obtained with
// /lua_tostring()/) in a new block, then executes /__VA_ARGS__/ in that block.
#define PU_VISIT_STR_AT(StackIndex_, StringRepr_, Var_, ...) \
    do { \
        PU_CHECK_TYPE_AT(StackIndex_, StringRepr_, LUA_TSTRING); \
        const char *Var_ = lua_tostring(PU_L, StackIndex_); \
        __VA_ARGS__ \
    } while (0)

// Checks that the object at the index /StackIndex_/ in /PU_L/'s stack is a string.
// Creates a /const char */ variable named /SVar_/ with the value of that string and a /size_t/
// variable named /NVar_/ with the length of that string (as if obtained with /lua_tolstring()/),
// then executes /__VA_ARGS__/ in that block.
#define PU_VISIT_LSTR_AT(StackIndex_, StringRepr_, SVar_, NVar_, ...) \
    do { \
        PU_CHECK_TYPE_AT(StackIndex_, StringRepr_, LUA_TSTRING); \
        size_t NVar_; \
        const char *SVar_ = lua_tolstring(PU_L, StackIndex_, &NVar_); \
        __VA_ARGS__ \
    } while (0)

// Checks that the object at the index /StackIndex_/ in /PU_L/'s stack is a number.
// Creates a /lua_Number/ variable named /Var_/ with the value of that number (as if obtained with
// /lua_tonumber()/), then executes /__VA_ARGS__/ in that block.
#define PU_VISIT_NUM_AT(StackIndex_, StringRepr_, Var_, ...) \
    do { \
        PU_CHECK_TYPE_AT(StackIndex_, StringRepr_, LUA_TNUMBER); \
        lua_Number Var_ = lua_tonumber(PU_L, StackIndex_); \
        __VA_ARGS__ \
    } while (0)

// Checks that the object at the index /StackIndex_/ in /PU_L/'s stack is a boolean.
// Creates a /bool/ variable named /Var_/ with the value of that number (as if obtained with
// /lua_toboolean()/), then executes /__VA_ARGS__/ in that block.
#define PU_VISIT_BOOL_AT(StackIndex_, StringRepr_, Var_, ...) \
    do { \
        PU_CHECK_TYPE_AT(StackIndex_, StringRepr_, LUA_TBOOLEAN); \
        bool Var_ = lua_toboolean(PU_L, StackIndex_); \
        __VA_ARGS__ \
    } while (0)

// Checks that the object at the index /StackIndex_/ in /PU_L/'s stack is a table.
// Then simply expands /__VA_ARGS__/.
// May be useful when you want not to just traverse a table. Dunno.
#define PU_VISIT_TABLE_AT(StackIndex_, StringRepr_, ...) \
    do { \
        PU_CHECK_TYPE_AT(StackIndex_, StringRepr_, LUA_TTABLE); \
        __VA_ARGS__ \
    } while (0)

// Checks that the object at the index /StackIndex_/ in /PU_L/'s stack is a table.
// Expands /LS_LUA_TRAVERSE(PU_L, StackIndex_)/ with /__VA_ARGS__) as a loop body.
#define PU_TRAVERSE_TABLE_AT(StackIndex_, StringRepr_, ...) \
    do { \
        PU_CHECK_TYPE_AT(StackIndex_, StringRepr_, LUA_TTABLE); \
        LS_LUA_TRAVERSE(PU_L, StackIndex_) { \
            __VA_ARGS__ \
        } \
    } while (0)

//------------------------------------------------------------------------------

// Please do not use these /PU__*/ macros directly.

// If the object at the index /StackIndex_/ in /PU_L/'s stack is not /nil/, executes
// /AtMacro_(StackIndex_, StringRepr_, __VA_ARGS__)/.
#define PU__MAYBE_EXPAND_AT(StackIndex_, StringRepr_, AtMacro_, ...) \
    do { \
        if (!lua_isnil(PU_L, StackIndex_)) { \
            AtMacro_(StackIndex_, StringRepr_, __VA_ARGS__); \
        } \
    } while (0)

// Obtains the object at the key /Key_/ of the table on top of /PU_L/'s stack, then executes
// /AtMacro_(-1, StringRepr_ ? StringRepr_ : Key_, __VA_ARGS__)/, then pops the object off.
// Note that /StringRepr_/ may be /NULL/.
#define PU__EXPAND_AT_KEY(Key_, StringRepr_, AtMacro_, ...) \
    do { \
        ls_lua_rawgetf(PU_L, Key_); \
        AtMacro_(-1, StringRepr_ ? StringRepr_ : Key_, __VA_ARGS__); \
        lua_pop(PU_L, 1); \
    } while (0)

// Obtains the object at the key /Key_/ of the table on top of /PU_L/'s stack; then, if it is not
// /nil/, executes /AtMacro_(-1, StringRepr_ ? StringRepr_ : Key_, __VA_ARGS__)/; then pops it off.
// Note that /StringRepr_/ may be /NULL/.
#define PU__MAYBE_EXPAND_AT_KEY(Key_, StringRepr_, AtMacro_, ...) \
    PU__EXPAND_AT_KEY(Key_, StringRepr_, PU__MAYBE_EXPAND_AT, AtMacro_, __VA_ARGS__)

//------------------------------------------------------------------------------

// Checks that the object at the key /Key_/ of the table on top of /PU_L/'s stack is a string.
// Creates a /const char */ variable named /Var_/ with the value of that string (as if obtained with
// /lua_tostring()/) in a new block, then executes /__VA_ARGS__/ in that block.
#define PU_VISIT_STR(Key_, StringRepr_, Var_, ...) \
    PU__EXPAND_AT_KEY(Key_, StringRepr_, PU_VISIT_STR_AT, Var_, __VA_ARGS__)

// Checks that the object at the key /Key_/ of the table on top of /PU_L/'s stack is a string.
// Creates a /const char */ variable named /SVar_/ with the value of that string and a /size_t/
// variable named /NVar_/ with the length of that string (as if obtained with /lua_tolstring()/),
// then executes /__VA_ARGS__/ in that block.
#define PU_VISIT_LSTR(Key_, StringRepr_, SVar_, NVar_, ...) \
    PU__EXPAND_AT_KEY(Key_, StringRepr_, PU_VISIT_LSTR_AT, SVar_, NVar_, __VA_ARGS__)

// Checks that the object at the key /Key_/ of the table on top of /PU_L/'s stack is a number.
// Creates a /lua_Number/ variable named /Var_/ with the value of that number (as if obtained with
// /lua_tonumber()/), then executes /__VA_ARGS__/ in that block.
#define PU_VISIT_NUM(Key_, StringRepr_, Var_, ...) \
    PU__EXPAND_AT_KEY(Key_, StringRepr_, PU_VISIT_NUM_AT, Var_, __VA_ARGS__)

// Checks that the object at the key /Key_/ of the table on top of /PU_L/'s stack is a boolean.
// Creates a /bool/ variable named /Var_/ with the value of that number (as if obtained with
// /lua_toboolean()/), then executes /__VA_ARGS__/ in that block.
#define PU_VISIT_BOOL(Key_, StringRepr_, Var_, ...) \
    PU__EXPAND_AT_KEY(Key_, StringRepr_, PU_VISIT_BOOL_AT, Var_, __VA_ARGS__)

// Checks that the object at the key /Key_/ of the table on top of /PU_L/'s stack is a table.
// Then simply expands /__VA_ARGS__/.
// May be useful when you want not to just traverse a table. Dunno.
#define PU_TRAVERSE_TABLE(Key_, StringRepr_, ...) \
    PU__EXPAND_AT_KEY(Key_, StringRepr_, PU_TRAVERSE_TABLE_AT, __VA_ARGS__)

// Checks that the object at the key /Key_/ of the table on top of /PU_L/'s stack is a table.
// Expands /LS_LUA_TRAVERSE(PU_L, StackIndex_)/ with /__VA_ARGS__) as a loop body.
#define PU_VISIT_TABLE(Key_, StringRepr_, ...) \
    PU__EXPAND_AT_KEY(Key_, StringRepr_, PU_VISIT_TABLE_AT, __VA_ARGS__)

//------------------------------------------------------------------------------

// These macros are essentially the same as their non-MAYBE conterparts, but do nothing if the
// object at the key /Key_/ of the table on top of /PU_L/'s stack is /nil/.

#define PU_MAYBE_VISIT_STR(Key_, StringRepr_, Var_, ...) \
    PU__MAYBE_EXPAND_AT_KEY(Key_, StringRepr_, PU_VISIT_STR_AT, Var_, __VA_ARGS__)

#define PU_MAYBE_VISIT_LSTR(Key_, StringRepr_, SVar_, NVar_, ...) \
    PU__MAYBE_EXPAND_AT_KEY(Key_, StringRepr_, PU_VISIT_LSTR_AT, SVar_, NVar_, __VA_ARGS__)

#define PU_MAYBE_VISIT_NUM(Key_, StringRepr_, Var_, ...) \
    PU__MAYBE_EXPAND_AT_KEY(Key_, StringRepr_, PU_VISIT_NUM_AT, Var_, __VA_ARGS__)

#define PU_MAYBE_VISIT_BOOL(Key_, StringRepr_, Var_, ...) \
    PU__MAYBE_EXPAND_AT_KEY(Key_, StringRepr_, PU_VISIT_BOOL_AT, Var_, __VA_ARGS__)

#define PU_MAYBE_VISIT_TABLE(Key_, StringRepr_, ...) \
    PU__MAYBE_EXPAND_AT_KEY(Key_, StringRepr_, PU_VISIT_TABLE_AT, __VA_ARGS__)

#define PU_MAYBE_TRAVERSE_TABLE(Key_, StringRepr_, ...) \
    PU__MAYBE_EXPAND_AT_KEY(Key_, StringRepr_, PU_TRAVERSE_TABLE_AT, __VA_ARGS__)

#endif
