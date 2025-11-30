/*
 * Copyright (C) 2025  luastatus developers
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

#include "escape_q.h"
#include <stdbool.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include "libls/ls_xallocf.h"
#include "my_error.h"
#include "describe_lua_err.h"

static bool push_compiled_thunk(
    lua_State *L,
    const char *prog,
    MyError *out_err)
{
    // L: -
    int rc = luaL_loadstring(L, prog);
    if (rc == 0) {
        // L: func
        return true;
    }
    // L: error_obj
    const char *err_kind;
    const char *err_msg;
    describe_lua_err(L, rc, &err_kind, &err_msg);
    my_error_printf(out_err, "Lua error: %s: %s", err_kind, err_msg);
    lua_pop(L, 1); // L: -
    return false;
}

bool escape_q_make_lfunc(
    lua_State *L,
    const char *repl_old,
    const char *repl_new,
    MyError *out_err)
{
    char *prog = ls_xallocf(
        "local s = ...\n"

        "if type(s) ~= 'string' then\n"
        "  error('argument #1: expected string, found ' .. type(s))\n"
        "end\n"

        "local r, _ = string.gsub(s, [[%s]], [[%s]])\n"
        "return r\n",
        repl_old,
        repl_new
    );

    bool ok = push_compiled_thunk(L, prog, out_err);

    free(prog);

    return ok;
}
