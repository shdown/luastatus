#ifndef event_watcher_h_
#define event_watcher_h_

#include "include/barlib_data.h"

LuastatusBarlibEWResult
event_watcher(LuastatusBarlibData *bd,
              LuastatusBarlibEWCallBegin call_begin,
              LuastatusBarlibEWCallEnd call_end);

#endif
