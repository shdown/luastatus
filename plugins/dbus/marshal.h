#ifndef marshal_h_
#define marshal_h_

#include <lua.h>
#include <glib.h>

void
marshal(lua_State *L, GVariant *var);

#endif
