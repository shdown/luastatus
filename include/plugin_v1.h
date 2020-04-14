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

#ifndef luastatus_include_plugin_v1_h_
#define luastatus_include_plugin_v1_h_

#include <lua.h>

#include "plugin_data_v1.h"
#include "common.h"

const int LUASTATUS_PLUGIN_LUA_VERSION_NUM = LUA_VERSION_NUM;

extern LuastatusPluginIface_v1 luastatus_plugin_iface_v1;

#endif
