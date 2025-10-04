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
#include <wchar.h>
#include <locale.h>

#include <lua.h>
#include <lauxlib.h>

#include "config.generated.h"

#if LUASTATUS_HAVE_WCWIDTH
static inline int my_wcwidth(wchar_t c)
{
    return wcwidth(c);
}
enum { IS_DUMMY_IMPLEMENTATION = 0 };
#else
static inline int my_wcwidth(wchar_t c)
{
    return c == L'\0' ? 0 : 1;
}
enum { IS_DUMMY_IMPLEMENTATION = 1 };
#endif

static inline size_t decode(mbstate_t *state, const char *s, size_t ns, wchar_t *out)
{
    size_t rc = mbrtowc(out, s, ns, state);
    if (rc == 0) {
        return 1;
    } else if (rc == (size_t) -1 || rc == (size_t) -2) {
        return -1;
    } else {
        return rc;
    }
}

size_t libwidechar_width(const char *s, size_t ns)
{
    size_t width = 0;
    mbstate_t state = {0};
    while (ns) {
        wchar_t c;
        size_t nbytes = decode(&state, s, ns, &c);
        if (nbytes == (size_t) -1) {
            return -1;
        }

        int w = my_wcwidth(c);
        if (w < 0 || c == L'\0') {
            return -1;
        }
        width += w;

        s += nbytes;
        ns -= nbytes;
    }
    return width;
}

size_t libwidechar_truncate_to_width(
        const char *s,
        size_t ns,
        size_t max_width,
        size_t *out_resut_width)
{
    const char *s_orig = s;
    mbstate_t state = {0};
    size_t width = 0;
    while (ns) {
        wchar_t c;
        size_t nbytes = decode(&state, s, ns, &c);
        if (nbytes == (size_t) -1) {
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

        s += nbytes;
        ns -= nbytes;
    }

    *out_resut_width = width;
    return s - s_orig;
}

void libwidechar_make_valid_and_printable(
        const char *s, size_t ns,
        const char *bad, size_t bad_len,
        void (*append)(void *ud, const char *ptr, size_t len),
        void *append_ud)
{
    mbstate_t state = {0};
    const char *prev = s;
    while (ns) {
        wchar_t c;
        size_t nbytes = decode(&state, s, ns, &c);
        if (nbytes == (size_t) -1) {
            append(append_ud, prev, s - prev);
            append(append_ud, bad, bad_len);
            ++s;
            --ns;
            prev = s;
            continue;
        }

        if (my_wcwidth(c) < 0 || c == L'\0') {
            append(append_ud, prev, s - prev);
            append(append_ud, bad, bad_len);
            s += nbytes;
            ns -= nbytes;
            prev = s;
            continue;
        }

        s += nbytes;
        ns -= nbytes;
    }
    append(append_ud, prev, s - prev);
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

static inline size_t nonneg_double_to_size_t(double d)
{
    static const double LIMIT =
#if SIZE_MAX <= UINT32_MAX
        SIZE_MAX
#else
        9223372036854775808.0 // 2^63
#endif
    ;

    if (d > LIMIT) {
        return SIZE_MAX;
    }
    return d;
}

static size_t extract_ij(lua_State *L, int pos, size_t if_absent)
{
    double d = luaL_optnumber(L, pos, -1);
    if (d >= 0) {
        return nonneg_double_to_size_t(d);
    }
    return if_absent;
}

static const char *extract_string_with_ij(lua_State *L, int str_pos, int i_pos, size_t *out_len)
{
    size_t ns;
    const char *s = luaL_checklstring(L, str_pos, &ns);

    size_t i = extract_ij(L, i_pos,     1);
    size_t j = extract_ij(L, i_pos + 1, SIZE_MAX);
    if (!i) {
        luaL_argerror(L, i_pos, "is zero (expected 1-based index)");
        // unreachable
        return NULL;
    }
    --i;

    if (j > ns) {
        j = ns;
    }
    if (i > ns) {
        i = ns;
    }
    if (j < i) {
        *out_len = 0;
        return NULL;
    }

    *out_len = j - i;
    return s + i;
}

static int lfunc_width(lua_State *L)
{
    size_t ns;
    const char *s = extract_string_with_ij(L, 1, 2, &ns);

    LocaleSavedData lsd = begin_locale(L);
    size_t res = libwidechar_width(s, ns);
    end_locale(lsd, L);

    // L: ?
    if (res == (size_t) -1) {
        lua_pushnil(L); // L: ? nil
    } else {
        lua_pushnumber(L, res); // ? L: width
    }
    return 1;
}

static int lfunc_truncate_to_width(lua_State *L)
{
    size_t ns;
    const char *s = extract_string_with_ij(L, 1, 3, &ns);

    double d_max_width = luaL_checknumber(L, 2);
    if (!(d_max_width >= 0)) {
        return luaL_argerror(L, 2, "negative or NaN");
    }
    size_t max_width = nonneg_double_to_size_t(d_max_width);

    LocaleSavedData lsd = begin_locale(L);
    size_t res_width;
    size_t res_len = libwidechar_truncate_to_width(s, ns, max_width, &res_width);
    end_locale(lsd, L);

    if (res_len == (size_t) -1) {
        lua_pushnil(L); // L: nil
        lua_pushnil(L); // L: nil nil
    } else {
        lua_pushlstring(L, s, res_len); // L: result
        lua_pushnumber(L, res_width); // L: result result_width
    }

    return 2;
}

static void append_to_lua_buf_callback(void *ud, const char *ptr, size_t len)
{
    luaL_Buffer *b = ud;
    luaL_addlstring(b, ptr, len);
}

static int lfunc_make_valid_and_printable(lua_State *L)
{
    size_t ns;
    const char *s = extract_string_with_ij(L, 1, 3, &ns);

    size_t nbad;
    const char *bad = luaL_checklstring(L, 2, &nbad);

    luaL_Buffer b;
    luaL_buffinit(L, &b);

    LocaleSavedData lsd = begin_locale(L);
    libwidechar_make_valid_and_printable(s, ns, bad, nbad, append_to_lua_buf_callback, &b);
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
