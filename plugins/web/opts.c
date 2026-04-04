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

#include "opts.h"
#include <stdbool.h>
#include <limits.h>
#include <math.h>
#include <lua.h>
#include "libls/ls_algo.h"
#include "libls/ls_lua_compat.h"
#include "libls/ls_panic.h"
#include "libls/ls_time_utils.h"
#include "set_error.h"
#include "next_request_params.h"

static bool apply_str(NextRequestParams *dst, lua_State *L, CURLoption which, char **out_errmsg)
{
    // L: ? something
    if (!lua_isstring(L, -1)) {
        set_type_error(out_errmsg, L, -1, LUA_TSTRING, "");
        return false;
    }

    const char *s = lua_tostring(L, -1);
    CURLcode rc = curl_easy_setopt(dst->C, which, (char *) s);
    if (rc != CURLE_OK) {
        set_curl_error(out_errmsg, rc);
        return false;
    }

    return true;
}

static bool apply_bool(NextRequestParams *dst, lua_State *L, CURLoption which, char **out_errmsg)
{
    // L: ? something
    if (!lua_isboolean(L, -1)) {
        set_type_error(out_errmsg, L, -1, LUA_TBOOLEAN, "");
        return false;
    }

    bool flag = lua_toboolean(L, -1);
    CURLcode rc = curl_easy_setopt(dst->C, which, (long) flag);
    if (rc != CURLE_OK) {
        set_curl_error(out_errmsg, rc);
        return false;
    }

    return true;
}

static bool fetch_int(
    lua_State *L,
    int *out_res,
    char **out_errmsg)
{
    // L: ? something
    if (!lua_isnumber(L, -1)) {
        set_type_error(out_errmsg, L, -1, LUA_TNUMBER, "");
        return false;
    }

    double fp = lua_tonumber(L, -1);

    if (isnan(fp)) {
        set_error(out_errmsg, "value is NaN");
        return false;
    }
    if (fp < 0) {
        *out_res = -1;
        return true;
    }
    if (fp > INT_MAX) {
        set_error(out_errmsg, "value is greater than INT_MAX (%d)", (int) INT_MAX);
        return false;
    }
    *out_res = (int) fp;
    return true;
}

static bool apply_long_nonneg(NextRequestParams *dst, lua_State *L, CURLoption which, char **out_errmsg)
{
    int val;
    if (!fetch_int(L, &val, out_errmsg)) {
        return false;
    }

    if (val < 0) {
        set_error(out_errmsg, "value is negative");
    }

    CURLcode rc = curl_easy_setopt(dst->C, which, (long) val);
    if (rc != CURLE_OK) {
        set_curl_error(out_errmsg, rc);
        return false;
    }
    return true;
}

static bool apply_long_nonneg_or_minus1(NextRequestParams *dst, lua_State *L, CURLoption which, char **out_errmsg)
{
    int val;
    if (!fetch_int(L, &val, out_errmsg)) {
        return false;
    }

    CURLcode rc = curl_easy_setopt(dst->C, which, (long) val);
    if (rc != CURLE_OK) {
        set_curl_error(out_errmsg, rc);
        return false;
    }
    return true;
}

static bool apply_headers(NextRequestParams *dst, lua_State *L, CURLoption which, char **out_errmsg)
{
    (void) which;

    // This must be impossible. Still, let's check.
    if (dst->headers != NULL) {
        set_error(out_errmsg, "the headers were passed multiple times, somehow");
        return false;
    }

    // L: ? something
    if (!lua_istable(L, -1)) {
        set_type_error(out_errmsg, L, -1, LUA_TTABLE, "");
        return false;
    }
    // L: ? table

    size_t n = ls_lua_array_len(L, -1);

    for (size_t i = 1; i <= n; ++i) {
        lua_rawgeti(L, -1, i); // L: ? table header

        if (!lua_isstring(L, -1)) {
            set_type_error(out_errmsg, L, -1, LUA_TSTRING, "array element: ");
            return false;
        }
        const char *s = lua_tostring(L, -1);
        dst->headers = curl_slist_append(dst->headers, s);
        if (!dst->headers) {
            LS_PANIC("curl_slist_append() returned NULL: out of memory");
        }

        lua_pop(L, 1); // L: ? table
    }

    CURLcode rc = curl_easy_setopt(dst->C, CURLOPT_HTTPHEADER, dst->headers);
    if (rc != CURLE_OK) {
        set_curl_error(out_errmsg, rc);
        return false;
    }
    return true;
}

static bool apply_timeout(NextRequestParams *dst, lua_State *L, CURLoption which, char **out_errmsg)
{
    (void) which;

    // L: ? value
    if (!lua_isnumber(L, -1)) {
        set_type_error(out_errmsg, L, -1, LUA_TNUMBER, "");
        return false;
    }

    double fp = lua_tonumber(L, -1);
    LS_TimeDelta TD;
    if (!ls_double_to_TD_checked(fp, &TD)) {
        set_error(out_errmsg, "invalid timeout");
        return false;
    }
    int ms = ls_TD_to_poll_ms_tmo(TD);
    if (ms < 0) {
        ms = INT_MAX;
    }

    CURLcode rc = curl_easy_setopt(dst->C, CURLOPT_TIMEOUT_MS, (long) ms);
    if (rc != CURLE_OK) {
        set_curl_error(out_errmsg, rc);
        return false;
    }

    return true;
}

const Opt OPTS[] = {
    {"url", apply_str, CURLOPT_URL},
    {"headers", apply_headers, 0},
    {"timeout", apply_timeout, 0},
    {"max_file_size", apply_long_nonneg, CURLOPT_MAXFILESIZE},

    {"auto_referer", apply_bool, CURLOPT_AUTOREFERER},
    {"custom_request", apply_str, CURLOPT_CUSTOMREQUEST},
    {"follow_location", apply_bool, CURLOPT_FOLLOWLOCATION},
    {"interface", apply_str, CURLOPT_INTERFACE},
    {"max_redirs", apply_long_nonneg_or_minus1, CURLOPT_MAXREDIRS},
    {"tcp_keepalive", apply_bool, CURLOPT_TCP_KEEPALIVE},

    {"proxy", apply_str, CURLOPT_PROXY},
    {"proxy_username", apply_str, CURLOPT_PROXYUSERNAME},
    {"proxy_password", apply_str, CURLOPT_PROXYPASSWORD},

    {"post_fields", apply_str, CURLOPT_COPYPOSTFIELDS},
};

const size_t OPTS_NUM = LS_ARRAY_SIZE(OPTS);
