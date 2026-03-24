#pragma once

#include <lua.h>
#include <glib.h>

// Returns a borrow.
const GVariantType *zoo_uncvt_type_fetch_borrow(lua_State *L, int pos, const char *what);

// Steals /t/.
void zoo_uncvt_type_bake_steal(GVariantType *t, lua_State *L);

void zoo_uncvt_type_register_mt_and_funcs(lua_State *L);
