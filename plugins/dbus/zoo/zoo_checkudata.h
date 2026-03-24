#pragma once

#include <lua.h>

void *zoo_checkudata(lua_State *L, int pos, const char *tname, const char *what);
