#pragma once

#include <lua.h>
#include <stdbool.h>

bool zoo_call_prot(lua_State *L, int args, int nresults, lua_CFunction f, void *f_ud);
