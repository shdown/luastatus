#pragma once

#include <lua.h>
#include <glib.h>

// Returns a new reference to the value.
// This reference is never a "floating reference".
GVariant *zoo_uncvt_val_fetch_newref(lua_State *L, int pos, const char *what);

void zoo_uncvt_val_register_mt_and_funcs(lua_State *L);
