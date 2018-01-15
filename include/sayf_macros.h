#ifndef luastatus_include_sayf_macros_
#define luastatus_include_sayf_macros_

#define LS_FATALF(Data_, ...)   Data_->sayf(Data_->userdata, LUASTATUS_LOG_FATAL,   __VA_ARGS__)
#define LS_ERRF(Data_, ...)     Data_->sayf(Data_->userdata, LUASTATUS_LOG_ERR,     __VA_ARGS__)
#define LS_WARNF(Data_, ...)    Data_->sayf(Data_->userdata, LUASTATUS_LOG_WARN,    __VA_ARGS__)
#define LS_INFOF(Data_, ...)    Data_->sayf(Data_->userdata, LUASTATUS_LOG_INFO,    __VA_ARGS__)
#define LS_VERBOSEF(Data_, ...) Data_->sayf(Data_->userdata, LUASTATUS_LOG_VERBOSE, __VA_ARGS__)
#define LS_DEBUGF(Data_, ...)   Data_->sayf(Data_->userdata, LUASTATUS_LOG_DEBUG,   __VA_ARGS__)
#define LS_TRACEF(Data_, ...)   Data_->sayf(Data_->userdata, LUASTATUS_LOG_TRACE,   __VA_ARGS__)

#endif
