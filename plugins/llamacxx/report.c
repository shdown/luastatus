/*
 * Copyright (C) 2026  luastatus developers
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

#include "report.h"

#include <stddef.h>
#include <lua.h>
#include "libls/ls_panic.h"
#include "include/plugin_data_v1.h"

static inline void push_value(lua_State *L, ReportField f)
{
    switch (f.type) {
    case REPORT_TYPE_CSTR:
        lua_pushstring(L, f.v);
        return;
    case REPORT_TYPE_LSTR:
        lua_pushlstring(L, f.v, f.nv);
        return;
    case REPORT_TYPE_BOOL_TRUE:
    case REPORT_TYPE_BOOL_FALSE:
        lua_pushboolean(L, f.type == REPORT_TYPE_BOOL_TRUE);
        return;
    }
    LS_MUST_BE_UNREACHABLE();
}

void report_generic(
    LuastatusPluginData *pd,
    LuastatusPluginRunFuncs funcs,
    const ReportField *fields,
    size_t nfields)
{
    lua_State *L = funcs.call_begin(pd->userdata);

    lua_createtable(L, 0, nfields); // L: table

    for (size_t i = 0; i < nfields; ++i) {
        ReportField f = fields[i];
        push_value(L, f); // L: table value
        lua_setfield(L, -2, f.k); // L: table
    }

    funcs.call_end(pd->userdata);
}
