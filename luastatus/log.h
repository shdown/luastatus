#ifndef log_h_
#define log_h_

#include <stdbool.h>
#include <stdarg.h>
#include "include/common.h"
#include "libls/compdep.h"

enum {
    DEFAULT_LOGLEVEL = LUASTATUS_INFO,
};

extern int global_loglevel;

const char *
loglevel_tostr(int level);

int
loglevel_fromstr(const char *s);

void
common_vsayf(int level, const char *subsystem, const char *fmt, va_list vl);

LS_INHEADER LS_ATTR_PRINTF(2, 3)
void
sayf(int level, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    common_vsayf(level, NULL, fmt, vl);
    va_end(vl);
}

#endif
