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

#include "parse_opts.h"
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include "libls/ls_panic.h"
#include "opts.h"
#include "next_request_params.h"
#include "set_error.h"
#include "make_request.h"

static int find_opt(const char *spelling)
{
    for (size_t i = 0; i < OPTS_NUM; ++i) {
        if (strcmp(spelling, OPTS[i].spelling) == 0) {
            return i;
        }
    }
    return -1;
}

static bool check_if_our_flag(NextRequestParams *dst, const char *s)
{
    if (strcmp(s, "with_headers") == 0) {
        dst->local_req_flags |= REQ_FLAG_NEEDS_HEADERS;
        return true;
    }
    if (strcmp(s, "debug") == 0) {
        dst->local_req_flags |= REQ_FLAG_DEBUG;
        return true;
    }
    return false;
}

static bool handle_option(NextRequestParams *dst, lua_State *L, char **out_errmsg, int *out_opt_idx)
{
    // L: ? key value
    if (!lua_isstring(L, -2)) {
        set_type_error(out_errmsg, L, -2, LUA_TSTRING, "table key: ");
        return false;
    }

    const char *s = lua_tostring(L, -2);
    if (check_if_our_flag(dst, s)) {
        *out_opt_idx = -1;
        return true;
    }

    int opt_idx = find_opt(s);
    if (opt_idx < 0) {
        set_error(out_errmsg, "unknown option '%s'", s);
        return false;
    }
    const Opt *opt = &OPTS[opt_idx];

    char *nested_errmsg;
    if (!opt->apply(dst, L, opt->which, &nested_errmsg)) {
        set_error(out_errmsg, "option '%s': %s", s, nested_errmsg);
        free(nested_errmsg);
        return false;
    }

    *out_opt_idx = opt_idx;
    return true;
}

bool parse_opts(NextRequestParams *dst, lua_State *L, char **out_errmsg)
{
    LS_ASSERT(dst->C != NULL);
    LS_ASSERT(dst->headers == NULL);

    LS_ASSERT(lua_istable(L, -1));

    bool got_required_option = false;

    // L: ? table
    lua_pushnil(L); // L: ? table nil
    while (lua_next(L, -2)) {
        // L: ? table key value

        int opt_idx;
        if (!handle_option(dst, L, out_errmsg, &opt_idx)) {
            return false;
        }
        if (opt_idx == REQUIRED_OPTION_INDEX) {
            got_required_option = true;
        }

        lua_pop(L, 1); // L: ? table key
    }
    // L: ? table

    if (!got_required_option) {
        const Opt *opt = &OPTS[REQUIRED_OPTION_INDEX];
        set_error(out_errmsg, "missing required option '%s'", opt->spelling);
        return false;
    }

    return true;
}
