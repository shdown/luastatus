#pragma once

#include <lua.h>

void zoo_mt_begin(lua_State *L, const char *mt_name);

void zoo_mt_add_method(lua_State *L, const char *name, lua_CFunction f);

void zoo_mt_end(lua_State *L);
