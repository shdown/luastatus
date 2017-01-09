#ifndef lua_libs_h_
#define lua_libs_h_

#include <lua.h>
#include "widget.h"
#include "barlib.h"

void
lualibs_inject(lua_State *L);

void
lualibs_register_funcs(Widget *w, Barlib *barlib);

#endif
