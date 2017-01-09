#ifndef luastatus_include_barlib_logf_macros_
#define luastatus_include_barlib_logf_macros_

#define LUASTATUS_FATALF(Data_, ...)   Data_->logf(Data_->userdata, LUASTATUS_FATAL, __VA_ARGS__)
#define LUASTATUS_ERRF(Data_, ...)     Data_->logf(Data_->userdata, LUASTATUS_ERR, __VA_ARGS__)
#define LUASTATUS_WARNF(Data_, ...)    Data_->logf(Data_->userdata, LUASTATUS_WARN, __VA_ARGS__)
#define LUASTATUS_INFOF(Data_, ...)    Data_->logf(Data_->userdata, LUASTATUS_INFO, __VA_ARGS__)
#define LUASTATUS_VERBOSEF(Data_, ...) Data_->logf(Data_->userdata, LUASTATUS_VERBOSE, __VA_ARGS__)
#define LUASTATUS_DEBUGF(Data_, ...)   Data_->logf(Data_->userdata, LUASTATUS_DEBUG, __VA_ARGS__)
#define LUASTATUS_TRACEF(Data_, ...)   Data_->logf(Data_->userdata, LUASTATUS_TRACE, __VA_ARGS__)

#endif
