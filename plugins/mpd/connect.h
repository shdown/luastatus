#ifndef connect_h_
#define connect_h_

#include "include/plugin_data.h"

int
socket_open(LuastatusPluginData *pd, const char *path);

int
tcp_open(LuastatusPluginData *pd, const char *hostname, const char *service);

#endif
