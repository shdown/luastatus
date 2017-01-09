#ifndef plugin_run_data_h_
#define plugin_run_data_h_

#include "include/plugin_data.h"

typedef struct {
    LuastatusPluginData *pd;
    LuastatusPluginCallBegin *call_begin;
    LuastatusPluginCallEnd *call_end;
} PluginRunData;

#endif
