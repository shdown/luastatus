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

#include "escape_quoted.h"
#include "my_error.h"
#include "describe_lua_err.h"
#include "libls/string_.h"
#include <stddef.h>
#include <stdbool.h>
#include <lua.h>
#include <lauxlib.h>

static bool push_compiled_chunk(
    lua_State *L,
    const char *repl_old,
    const char *repl_new,
    MyError *out_err)
{
    LS_String prog = ls_string_new_from_f(
        "local s = ...\n"
        "local r, _ = string.gsub(s, [[%s]], [[%s]])\n"
        "return r\n",
        repl_old,
        repl_new
    );
    ls_string_append_c(&prog, '\0');

    // L: -
    int rc = luaL_loadstring(L, prog.data);
    if (rc != LUA_OK) {
        // L: error_obj
        const char *err_kind;
        const char *err_msg;
        describe_lua_err(L, rc, &err_kind, &err_msg);
        my_error_printf(out_err, "Lua error: %s: %s", err_kind, err_msg);
        lua_pop(L, 1); // L: -
    }
    // else: L: func

    ls_string_free(prog);

    return rc == LUA_OK;
}

bool escape_quoted_register_all_lfuncs(lua_State *L, MyError *out_err)
{
    // NOTE: dq = double-quote ("), sq = single-quote ('), bq = backquote (`)

    // L: table

    // REPLACE: dq => sq+sq
    if (!push_compiled_chunk(L, "\"", "''", out_err)) {
        return false;
    }
    // L: table func
    lua_setfield(L, -2, "escape_double_quoted"); // L: table

    // REPLACE: sq => bq
    if (!push_compiled_chunk(L, "'", "`", out_err)) {
        return false;
    }
    // L: table func
    lua_setfield(L, -2, "escape_single_quoted"); // L: table

    return true;
}
