#ifndef luastatus_include_sayf_macros_
#define luastatus_include_sayf_macros_

#define LS_FATALF(Data_, ...)   Data_->sayf(Data_->userdata, LUASTATUS_FATAL,   __VA_ARGS__)
#define LS_ERRF(Data_, ...)     Data_->sayf(Data_->userdata, LUASTATUS_ERR,     __VA_ARGS__)
#define LS_WARNF(Data_, ...)    Data_->sayf(Data_->userdata, LUASTATUS_WARN,    __VA_ARGS__)
#define LS_INFOF(Data_, ...)    Data_->sayf(Data_->userdata, LUASTATUS_INFO,    __VA_ARGS__)
#define LS_VERBOSEF(Data_, ...) Data_->sayf(Data_->userdata, LUASTATUS_VERBOSE, __VA_ARGS__)
#define LS_DEBUGF(Data_, ...)   Data_->sayf(Data_->userdata, LUASTATUS_DEBUG,   __VA_ARGS__)
#define LS_TRACEF(Data_, ...)   Data_->sayf(Data_->userdata, LUASTATUS_TRACE,   __VA_ARGS__)

#endif
