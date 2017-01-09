#ifndef luastatus_include_loglevel_h_
#define luastatus_include_loglevel_h_

typedef enum {
    LUASTATUS_FATAL,
    LUASTATUS_ERR,
    LUASTATUS_WARN,
    LUASTATUS_INFO,
    LUASTATUS_VERBOSE,
    LUASTATUS_DEBUG,
    LUASTATUS_TRACE,

    LUASTATUS_LOGLEVEL_LAST,
} LuastatusLogLevel;

#endif
