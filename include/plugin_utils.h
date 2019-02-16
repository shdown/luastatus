#ifndef luastatus_include_plugin_utils_h_
#define luastatus_include_plugin_utils_h_

// A number of macros that simplify marshalling widget options for plugins.

#include <stddef.h>
#include <stdbool.h>
#include <lua.h>
#include <lauxlib.h>

#include "libls/lua_utils.h"

#include "common.h"

#define PU_CHECK_TYPE(Idx_, Repr_, LuaType_) \
    do { \
        if (lua_type(L, Idx_) != LuaType_) { \
            pd->sayf(pd->userdata, LUASTATUS_LOG_FATAL, "%s: expected %s, found %s", \
                     Repr_, lua_typename(L, LuaType_), luaL_typename(L, Idx_)); \
            goto error; \
        } \
    } while (0)

//------------------------------------------------------------------------------

#define PU_VISIT_STR(Idx_, Repr_, Var_, ...) \
    do { \
        PU_CHECK_TYPE(Idx_, Repr_, LUA_TSTRING); \
        const char *Var_ = lua_tostring(L, -1); \
        __VA_ARGS__ \
    } while (0)

#define PU_VISIT_STR_FIELD(TableIdx_, Key_, Repr_, Var_, ...) \
    do { \
        lua_getfield(L, TableIdx_, Key_); /* L: ? value */ \
        PU_VISIT_STR(-1, Repr_, Var_, __VA_ARGS__); \
        lua_pop(L, 1); /* L: ? */ \
    } while (0)

#define PU_MAYBE_VISIT_STR_FIELD(TableIdx_, Key_, Repr_, Var_, ...) \
    do { \
        lua_getfield(L, TableIdx_, Key_); /* L: ? value */ \
        if (!lua_isnil(L, -1)) { \
            PU_VISIT_STR(-1, Repr_, Var_, __VA_ARGS__); \
        } \
        lua_pop(L, 1); /* L: ? */ \
    } while (0)

//------------------------------------------------------------------------------

#define PU_VISIT_LSTR(Idx_, Repr_, SVar_, NVar_, ...) \
    do { \
        PU_CHECK_TYPE(Idx_, Repr_, LUA_TSTRING); \
        size_t NVar_; \
        const char *SVar_ = lua_tolstring(L, -1, &NVar_); \
        __VA_ARGS__ \
    } while (0)

#define PU_VISIT_LSTR_FIELD(TableIdx_, Key_, Repr_, SVar_, NVar_, ...) \
    do { \
        lua_getfield(L, TableIdx_, Key_); /* L: ? value */ \
        PU_VISIT_LSTR(-1, Repr_, SVar_, NVar_, __VA_ARGS__); \
        lua_pop(L, 1); /* L: ? */ \
    } while (0)

#define PU_MAYBE_VISIT_LSTR_FIELD(TableIdx_, Key_, Repr_, SVar_, NVar_, ...) \
    do { \
        lua_getfield(L, TableIdx_, Key_); /* L: ? value */ \
        if (!lua_isnil(L, -1)) { \
            PU_VISIT_LSTR(-1, Repr_, SVar_, NVar_, __VA_ARGS__); \
        } \
        lua_pop(L, 1); /* L: ? */ \
    } while (0)

//------------------------------------------------------------------------------

#define PU_VISIT_NUM(Idx_, Repr_, Var_, ...) \
    do { \
        PU_CHECK_TYPE(Idx_, Repr_, LUA_TNUMBER); \
        lua_Number Var_ = lua_tonumber(L, -1); \
        __VA_ARGS__ \
    } while (0)

#define PU_VISIT_NUM_FIELD(TableIdx_, Key_, Repr_, Var_, ...) \
    do { \
        lua_getfield(L, TableIdx_, Key_); /* L: ? value */ \
        PU_VISIT_NUM(-1, Repr_, Var_, __VA_ARGS__); \
        lua_pop(L, 1); /* L: ? */ \
    } while (0)

#define PU_MAYBE_VISIT_NUM_FIELD(TableIdx_, Key_, Repr_, Var_, ...) \
    do { \
        lua_getfield(L, TableIdx_, Key_); /* L: ? value */ \
        if (!lua_isnil(L, -1)) { \
            PU_VISIT_NUM(-1, Repr_, Var_, __VA_ARGS__); \
        } \
        lua_pop(L, 1); /* L: ? */ \
    } while (0)

//------------------------------------------------------------------------------

#define PU_VISIT_BOOL(Idx_, Repr_, Var_, ...) \
    do { \
        PU_CHECK_TYPE(Idx_, Repr_, LUA_TBOOLEAN); \
        bool Var_ = lua_toboolean(L, -1); \
        __VA_ARGS__ \
    } while (0)

#define PU_VISIT_BOOL_FIELD(TableIdx_, Key_, Repr_, Var_, ...) \
    do { \
        lua_getfield(L, TableIdx_, Key_); /* L: ? value */ \
        PU_VISIT_BOOL(-1, Repr_, Var_, __VA_ARGS__); \
        lua_pop(L, 1); /* L: ? */ \
    } while (0)

#define PU_MAYBE_VISIT_BOOL_FIELD(TableIdx_, Key_, Repr_, Var_, ...) \
    do { \
        lua_getfield(L, TableIdx_, Key_); /* L: ? value */ \
        if (!lua_isnil(L, -1)) { \
            PU_VISIT_BOOL(-1, Repr_, Var_, __VA_ARGS__); \
        } \
        lua_pop(L, 1); /* L: ? */ \
    } while (0)

//------------------------------------------------------------------------------

#define PU_VISIT_TABLE(Idx_, Repr_, ...) \
    do { \
        PU_CHECK_TYPE(Idx_, Repr_, LUA_TTABLE); \
        LS_LUA_TRAVERSE(L, Idx_) { \
            __VA_ARGS__ \
        } \
    } while (0)

#define PU_VISIT_TABLE_FIELD(TableIdx_, Key_, Repr_, ...) \
    do { \
        lua_getfield(L, TableIdx_, Key_); /* L: ? value */ \
        PU_VISIT_TABLE(-1, Repr_, __VA_ARGS__); \
        lua_pop(L, 1); /* L: ? */ \
    } while (0)

#define PU_MAYBE_VISIT_TABLE_FIELD(TableIdx_, Key_, Repr_, ...) \
    do { \
        lua_getfield(L, TableIdx_, Key_); /* L: ? value */ \
        if (!lua_isnil(L, -1)) { \
            PU_VISIT_TABLE(-1, Repr_, __VA_ARGS__); \
        } \
        lua_pop(L, 1); /* L: ? */ \
    } while (0)

//------------------------------------------------------------------------------

#endif
