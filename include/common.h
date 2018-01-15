#ifndef luastatus_include_common_h_
#define luastatus_include_common_h_

enum {
    LUASTATUS_OK,
    LUASTATUS_ERR,
    LUASTATUS_NONFATAL_ERR,
};

enum {
    LUASTATUS_LOG_FATAL,
    LUASTATUS_LOG_ERR,
    LUASTATUS_LOG_WARN,
    LUASTATUS_LOG_INFO,
    LUASTATUS_LOG_VERBOSE,
    LUASTATUS_LOG_DEBUG,
    LUASTATUS_LOG_TRACE,

    LUASTATUS_LOG_LAST,
};

#endif
