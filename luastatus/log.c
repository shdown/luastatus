#include "log.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "libls/sprintf_utils.h"
#include "libls/compdep.h"

LuastatusLogLevel global_loglevel = DEFAULT_LOGLEVEL;

#define EXPAND_LOGLEVELS() \
    X(LUASTATUS_FATAL,   "fatal") \
    X(LUASTATUS_ERR,     "error") \
    X(LUASTATUS_WARN,    "warning") \
    X(LUASTATUS_INFO,    "info") \
    X(LUASTATUS_VERBOSE, "verbose") \
    X(LUASTATUS_DEBUG,   "debug") \
    X(LUASTATUS_TRACE,   "trace")

const char *
loglevel_tostr(LuastatusLogLevel level)
{
    switch (level) {
#define X(Level_, Name_) case Level_: return Name_;
    EXPAND_LOGLEVELS()
#undef X
    case LUASTATUS_LOGLEVEL_LAST:
        break;
    }
    LS_UNREACHABLE();
}

LuastatusLogLevel
loglevel_fromstr(const char *s)
{
#define X(Level_, Name_) \
    if (strcmp(s, Name_) == 0) { \
        return Level_; \
    }
    EXPAND_LOGLEVELS()
#undef X
    return LUASTATUS_LOGLEVEL_LAST;
}

#undef EXPAND_LOGLEVELS

void
common_logf(LuastatusLogLevel level, const char *subsystem, const char *fmt, va_list vl)
{
    if (level > global_loglevel) {
        return;
    }

    char buf[1024];
    ls_svsnprintf(buf, sizeof(buf), fmt, vl);

    // http://pubs.opengroup.org/onlinepubs/9699919799/functions/flockfile.html
    //
    // > All functions that reference (FILE *) objects, except those with names ending in _unlocked,
    // > shall behave as if they use flockfile() and funlockfile() internally to obtain ownership of
    // > these (FILE *) objects.
    //
    // So fprintf is guaranteed to be atomic.

    if (subsystem) {
        fprintf(stderr, "luastatus: (%s) %s: %s\n", subsystem, loglevel_tostr(level), buf);
    } else {
        fprintf(stderr, "luastatus: %s: %s\n", loglevel_tostr(level), buf);
    }
}

void
internal_logf(LuastatusLogLevel level, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    common_logf(level, NULL, fmt, vl);
    va_end(vl);
}
