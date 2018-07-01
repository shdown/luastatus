#ifndef connect_h_
#define connect_h_

#include "include/plugin_data_v1.h"

int
unixdom_open(LuastatusPluginData *pd, const char *path);

int
inetdom_open(LuastatusPluginData *pd, const char *hostname, const char *service);

#endif
