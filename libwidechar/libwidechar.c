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

#define _XOPEN_SOURCE 500

#include "libwidechar.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <wchar.h>
#include <locale.h>

#include <lua.h>
#include <lauxlib.h>

#include "libwidechar_xspan.h"
#include "config.generated.h"

#if LUASTATUS_HAVE_WCWIDTH

static inline int my_wcwidth(wchar_t c) { return wcwidth(c); }
enum { IS_DUMMY_IMPLEMENTATION = 0 };

#else

static inline int my_wcwidth(wchar_t c) { return c == L'\0' ? 0 : 1; }
enum { IS_DUMMY_IMPLEMENTATION = 1 };

#endif

static inline bool next(mbstate_t *state, xspan x, xspan *new_x, wchar_t *out)
{
    SAFEV unproc_v = xspan_unprocessed_v(x);

    size_t rc = mbrtowc(out, SAFEV_ptr_UNSAFE(unproc_v), SAFEV_len(unproc_v), state);
    if ((rc == (size_t) -1) || (rc == (size_t) -2)) {
        return false;
    }
    size_t n = rc == 0 ? 1 : rc;
    *new_x = x;
    xspan_advance(new_x, n);
    return true;
}

int libwidechar_width(SAFEV v, uint64_t *out_width)
{
    xspan x = v_to_xspan(v);
    uint64_t width = 0;
    mbstate_t state = {0};
    while (xspan_nonempty(x)) {
        wchar_t c;
        if (!next(&state, x, &x, &c)) {
            return -1;
        }
        int w = my_wcwidth(c);
        if (w < 0 || c == L'\0') {
            return -1;
        }
        width += w;
    }
    *out_width = width;
    return 0;
}

size_t libwidechar_truncate_to_width(
        SAFEV v,
        uint64_t max_width,
        uint64_t *out_resut_width)
{
    xspan x = v_to_xspan(v);
    mbstate_t state = {0};
    uint64_t width = 0;
    while (xspan_nonempty(x)) {
        xspan new_x;
        wchar_t c;
        if (!next(&state, x, &new_x, &c)) {
            return -1;
        }

        int w = my_wcwidth(c);
        if (w < 0 || c == L'\0') {
            return -1;
        }

        width += w;
        if (width > max_width) {
            width -= w;
            break;
        }
        x = new_x;
    }

    *out_resut_width = width;
    return xspan_processed_len(x);
}

void libwidechar_make_valid_and_printable(
        SAFEV v,
        SAFEV bad,
        void (*append)(void *ud, SAFEV segment),
        void *append_ud)
{
    xspan x = v_to_xspan(v);
    mbstate_t state = {0};

    while (xspan_nonempty(x)) {
        xspan new_x;
        wchar_t c;
        if (next(&state, x, &new_x, &c)) {
            if (my_wcwidth(c) >= 0 && c != L'\0') {
                // this wchar_t is good
                x = new_x;
            } else {
                // valid, but non-printable
                append(append_ud, xspan_processed_v(x));
                append(append_ud, bad);
                x = xspan_skip_processed(new_x);
            }
        } else {
            // invalid
            append(append_ud, xspan_processed_v(x));
            append(append_ud, bad);
            xspan_advance(&x, 1);
            x = xspan_skip_processed(x);
        }
    }
    append(append_ud, xspan_processed_v(x));
}

typedef struct {
    locale_t native;
    locale_t old;
} LocaleSavedData;

static inline LocaleSavedData begin_locale(lua_State *L)
{
    LocaleSavedData lsd = {0};
    const char *err_msg;

    lsd.native = newlocale(LC_ALL_MASK, "", (locale_t) 0);
    if (lsd.native == (locale_t) 0) {
        err_msg = "begin_locale: newlocale() failed";
        goto fail;
    }
    lsd.old = uselocale(lsd.native);
    if (lsd.old == (locale_t) 0) {
        err_msg = "begin_locale: uselocale() failed";
        goto fail;
    }

    return lsd;

fail:
    if (lsd.native != (locale_t) 0) {
        freelocale(lsd.native);
    }
    luaL_error(L, "%s", err_msg);
    // unreachable
    return (LocaleSavedData) {0};
}

static inline void end_locale(LocaleSavedData lsd, lua_State *L)
{
    if (uselocale(lsd.old) == (locale_t) 0) {
        luaL_error(L, "end_locale: uselocale() failed");
    }
    freelocale(lsd.native);
}

static inline SAFEV v_from_lua_string(lua_State *L, int pos)
{
    size_t ns;
    const char *s = lua_tolstring(L, pos, &ns);
    return SAFEV_new_UNSAFE(s, ns);
}

static inline uint64_t nonneg_double_to_u64(double d)
{
    if (d >= 18446744073709551616.0) {
        return -1;
    }
    return d;
}

static inline size_t nonneg_double_to_size_t(double d)
{
    static const double LIMIT =
#if SIZE_MAX > UINT32_MAX
        9223372036854775808.0 // 2^63
#else
        SIZE_MAX
#endif
    ;
    if (d > LIMIT) {
        return -1;
    }
    return d;
}

static size_t extract_ij(lua_State *L, int pos, size_t if_absent)
{
    double d = luaL_optnumber(L, pos, -1);
    if (isgreaterequal(d, 0.0)) {
        return nonneg_double_to_size_t(d);
    }
    return if_absent;
}

static SAFEV extract_string_with_ij(lua_State *L, int str_pos, int i_pos)
{
    SAFEV v = v_from_lua_string(L, str_pos);

    size_t i = extract_ij(L, i_pos,     1);
    size_t j = extract_ij(L, i_pos + 1, SIZE_MAX);
    if (!i) {
        luaL_argerror(L, i_pos, "is zero (expected 1-based index)");
        // unreachable
        return (SAFEV) {0};
    }

    // convert one-based index into zero-based
    --i;
    // 'j' doesn't need to be converted because in Lua function, it is *inclusive*
    // index, but is instead used as an *exclusive* in the code below.

    i = SAFEV_trim_to_len(v, i);
    j = SAFEV_trim_to_len(v, j);

    if (j < i) {
        j = i;
    }

    return SAFEV_subspan(v, i, j);
}

static int lfunc_width(lua_State *L)
{
    SAFEV v = extract_string_with_ij(L, 1, 2);

    LocaleSavedData lsd = begin_locale(L);
    uint64_t width;
    int rc = libwidechar_width(v, &width);
    end_locale(lsd, L);

    // L: ?
    if (rc < 0) {
        lua_pushnil(L); // L: ? nil
    } else {
        lua_pushnumber(L, width); // ? L: width
    }
    return 1;
}

static int lfunc_truncate_to_width(lua_State *L)
{
    SAFEV v = extract_string_with_ij(L, 1, 3);

    double d_max_width = luaL_checknumber(L, 2);
    if (!isgreaterequal(d_max_width, 0.0)) {
        return luaL_argerror(L, 2, "negative or NaN");
    }
    uint64_t max_width = nonneg_double_to_u64(d_max_width);

    LocaleSavedData lsd = begin_locale(L);
    uint64_t res_width;
    size_t res_len = libwidechar_truncate_to_width(v, max_width, &res_width);
    end_locale(lsd, L);

    if (res_len == (size_t) -1) {
        lua_pushnil(L); // L: nil
        lua_pushnil(L); // L: nil nil
    } else {
        SAFEV res = SAFEV_subspan(v, 0, res_len);
        lua_pushlstring(L, SAFEV_ptr_UNSAFE(res), SAFEV_len(res)); // L: result
        lua_pushnumber(L, res_width); // L: result result_width
    }

    return 2;
}

static void append_to_lua_buf_callback(void *ud, SAFEV segment)
{
    luaL_Buffer *b = ud;
    luaL_addlstring(b, SAFEV_ptr_UNSAFE(segment), SAFEV_len(segment));
}

static int lfunc_make_valid_and_printable(lua_State *L)
{
    SAFEV v = extract_string_with_ij(L, 1, 3);

    SAFEV bad = v_from_lua_string(L, 2);

    luaL_Buffer b;
    luaL_buffinit(L, &b);

    LocaleSavedData lsd = begin_locale(L);
    libwidechar_make_valid_and_printable(v, bad, append_to_lua_buf_callback, &b);
    end_locale(lsd, L);

    luaL_pushresult(&b); // L: result

    return 1;
}

static int lfunc_is_dummy_implementation(lua_State *L)
{
    lua_pushboolean(L, IS_DUMMY_IMPLEMENTATION); // L: bool
    return 1;
}

void libwidechar_register_lua_funcs(lua_State *L)
{
    // L: table

    lua_pushcfunction(L, lfunc_width); // L: table func
    lua_setfield(L, -2, "width"); // L: table

    lua_pushcfunction(L, lfunc_truncate_to_width); // L: table func
    lua_setfield(L, -2, "truncate_to_width"); // L: table

    lua_pushcfunction(L, lfunc_make_valid_and_printable); // L: table func
    lua_setfield(L, -2, "make_valid_and_printable"); // L: table

    lua_pushcfunction(L, lfunc_is_dummy_implementation); // L: table func
    lua_setfield(L, -2, "is_dummy_implementation"); // L: table
}
