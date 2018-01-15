#ifndef luastatus_include_plugin_v1_h_
#define luastatus_include_plugin_v1_h_

#include <lua.h>

#include "plugin_data_v1.h"
#include "common.h"

const int LUASTATUS_PLUGIN_LUA_VERSION_NUM = LUA_VERSION_NUM;

extern LuastatusPluginIface_v1 luastatus_plugin_iface_v1;

#endif
