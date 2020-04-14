/*
 * Copyright (C) 2015-2020  luastatus developers
 *
 * This file is part of luastatus.
 *
 * luastatus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * luastatus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with luastatus.  If not, see <https://www.gnu.org/licenses/>.
 */

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
