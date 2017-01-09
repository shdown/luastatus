#ifndef plugin_h_
#define plugin_h_

#include <stdbool.h>
#include "include/plugin_data.h"

typedef struct {
    LuastatusPluginIface iface;
    char *name;
    void *dlhandle;
} Plugin;

bool
plugin_load(Plugin *p, const char *filename, const char *name);

void
plugin_unload(Plugin *p);

#endif
