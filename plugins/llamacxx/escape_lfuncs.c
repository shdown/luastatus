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

#include "escape_lfuncs.h"
#include "my_error.h"
#include "describe_lua_err.h"
#include "libls/ls_xallocf.h"
#include <stdlib.h>
#include <stdbool.h>
#include <lua.h>
#include <lauxlib.h>

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

static bool push_compiled_thunk_quote_esc(
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

bool register_all_escape_lfuncs(lua_State *L, MyError *out_err)
{
    // ======================================================================
    // NOTE: dq = double-quote ("), sq = single-quote ('), bq = backquote (`)
    // ======================================================================

    // L: table

    // ======================================================================
    // REPLACE: dq => sq+sq
    // ======================================================================
    if (!push_compiled_thunk_quote_esc(L, "\"", "''", out_err)) {
        return false;
    }
    // L: table func
    lua_setfield(L, -2, "escape_double_quoted"); // L: table

    // ======================================================================
    // REPLACE: sq => bq
    // ======================================================================
    if (!push_compiled_thunk_quote_esc(L, "'", "`", out_err)) {
        return false;
    }
    // L: table func
    lua_setfield(L, -2, "escape_single_quoted"); // L: table

    // ======================================================================
    // Escape for JSON
    // ======================================================================

    // Notes:
    //
    //  * Support for embedded NUL character ('\0') in gsub patterns is
    //    inconsistent between Lua versions and/or implementations:
    //
    //      $ lua5.1 -e 'print(("test"):gsub("\0", "x"))'
    //      xtxexsxtx 5
    //
    //      $ lua5.4 -e 'print(("test"):gsub("\0", "x"))'
    //      test  0
    //
    //    So we simply ignore everything after the first NUL character; this
    //    can be done "portably" across different versions/implementations.
    //
    //  * We embed non-printables directly into strings within Lua code.
    //    This is because Lua doesn't support \xHH escapes, but does
    //    support "stray" non-printables in strings.

#define JSON_ESC_PATTERN \
    /*[pattern start]*/ "[" \
    /*non-printables*/  "\x01-\x1f" \
    /*doule quote*/     "\"" \
    /*backslash*/       "\\\\" \
    /*forward slash*/   "/" \
    /*[pattern end]*/   "]"

    static const char *JSON_ESC_PROGRAM =
        "local s = ...\n"

        "if type(s) ~= 'string' then\n"
        "  error('argument #1: expected string, found ' .. type(s))\n"
        "end\n"

        "local iZ = string.find(s, '\\0')\n"
        "if iZ then\n"
        "  s = string.sub(s, 1, iZ - 1)\n"
        "end\n"

        "local r, _ = string.gsub(s, '" JSON_ESC_PATTERN "', function(c)\n"
        "  return string.format('\\\\u00%02x', string.byte(c))\n"
        "end)\n"
        "return r\n"
        ;

#undef JSON_ESC_PATTERN

    if (!push_compiled_thunk(L, JSON_ESC_PROGRAM, out_err)) {
        return false;
    }
    // L: table func
    lua_setfield(L, -2, "json_escape"); // L: table

    return true;
}
