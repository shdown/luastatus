#pragma once

#include <stdbool.h>
#include <lua.h>
#include "include/plugin_data_v1.h"

bool load_lualib(LuastatusPluginData *pd, lua_State *L);
