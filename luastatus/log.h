#ifndef log_h_
#define log_h_

#include <stdbool.h>
#include <stdarg.h>
#include "include/loglevel.h"
#include "libls/compdep.h"

enum {
    DEFAULT_LOGLEVEL = LUASTATUS_INFO,
};

extern LuastatusLogLevel global_loglevel;

const char *
loglevel_tostr(LuastatusLogLevel level);

LuastatusLogLevel
loglevel_fromstr(const char *s);

void
common_logf(LuastatusLogLevel level, const char *subsystem, const char *fmt, va_list vl);

LS_ATTR_PRINTF(2, 3)
void
internal_logf(LuastatusLogLevel level, const char *fmt, ...);

#endif
