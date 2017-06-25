#ifndef luastatus_include_common_h_
#define luastatus_include_common_h_

enum {
    LUASTATUS_RES_OK,
    LUASTATUS_RES_ERR,
    LUASTATUS_RES_NONFATAL_ERR,
};

enum {
    LUASTATUS_FATAL,
    LUASTATUS_ERR,
    LUASTATUS_WARN,
    LUASTATUS_INFO,
    LUASTATUS_VERBOSE,
    LUASTATUS_DEBUG,
    LUASTATUS_TRACE,

    LUASTATUS_LOGLEVEL_LAST,
};

#endif
