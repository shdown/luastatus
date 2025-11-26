/*
 * Copyright (C) 2015-2025  luastatus developers
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

#include "moonvisit.h"

#include <lua.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static void oom_handler(void)
{
    fputs("FATAL ERROR: out of memory (libmoonvisit).\n", stderr);
    abort();
}

// Copies zero-terminated 'src' into buffer 'dst' of size 'ndst', possibly truncating, but always
// zero-terminating the output (unless 'ndst' is 0, of course).
//
// Returns number of bytes written, not including terminating NUL.
static size_t write_str(char *dst, size_t ndst, const char *src)
{
    if (!ndst) {
        return 0;
    }
    size_t ncopy = strnlen(src, ndst - 1);
    memcpy(dst, src, ncopy);
    dst[ncopy] = '\0';
    return ncopy;
}

bool moon_visit_err(MoonVisit *mv, const char *fmt, ...)
{
    size_t off = 0;
    if (mv->where) {
        off += write_str(mv->errbuf + off, mv->nerrbuf - off, mv->where);
        off += write_str(mv->errbuf + off, mv->nerrbuf - off, ": ");
    }

    va_list vl;
    va_start(vl, fmt);
    int r = vsnprintf(mv->errbuf + off, mv->nerrbuf - off, fmt, vl);
    va_end(vl);

    if (r < 0) {
        mv->errbuf[0] = '\0';
        return false;
    }
    return true;
}

int moon_visit_checktype_at(
    MoonVisit *mv,
    const char *what,
    int pos,
    int type)
{
    int got_type = lua_type(mv->L, pos);
    if (got_type != type) {
        if (what) {
            moon_visit_err(
                mv, "%s: expected %s, found %s",
                what,
                lua_typename(mv->L, type),
                lua_typename(mv->L, got_type));
        } else {
            moon_visit_err(
                mv, "expected %s, found %s",
                lua_typename(mv->L, type),
                lua_typename(mv->L, got_type));
        }
        return -1;
    }
    return 0;
}

int moon_visit_str_f(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    int (*f)(MoonVisit *mv, void *ud, const char *s, size_t ns),
    void *ud,
    bool skip_nil)
{
    int top = lua_gettop(mv->L);
    int r;

    lua_getfield(mv->L, table_pos, key);

    if (skip_nil && lua_isnil(mv->L, -1)) {
        r = 0;
        goto done;
    }

    if (moon_visit_checktype_at(mv, key, -1, LUA_TSTRING) < 0) {
        r = -1;
        goto done;
    }

    size_t ns;
    const char *s = lua_tolstring(mv->L, -1, &ns);

    const char *old_where = mv->where;
    r = f(mv, ud, s, ns);
    mv->where = old_where;

done:
    lua_settop(mv->L, top);
    return r;
}

int moon_visit_str(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    char **ps,
    size_t *pn,
    bool skip_nil)
{
    int r;
    lua_getfield(mv->L, table_pos, key);
    if (skip_nil && lua_isnil(mv->L, -1)) {
        r = 0;
        goto done;
    }
    if (moon_visit_checktype_at(mv, key, -1, LUA_TSTRING) < 0) {
        r = -1;
        goto done;
    }

    size_t ns;
    const char *s = lua_tolstring(mv->L, -1, &ns);

    char *buf = malloc(ns + 1);
    if (!buf) {
        oom_handler();
    }
    memcpy(buf, s, ns + 1);

    *ps = buf;
    if (pn) {
        *pn = ns;
    }
    r = 1;

done:
    lua_pop(mv->L, 1);
    return r;
}

int moon_visit_num(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    double *p,
    bool skip_nil)
{
    int r;
    lua_getfield(mv->L, table_pos, key);
    if (skip_nil && lua_isnil(mv->L, -1)) {
        r = 0;
        goto done;
    }
    if (moon_visit_checktype_at(mv, key, -1, LUA_TNUMBER) < 0) {
        r = -1;
        goto done;
    }

    *p = lua_tonumber(mv->L, -1);
    r = 1;

done:
    lua_pop(mv->L, 1);
    return r;
}

int moon_visit_bool(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    bool *p,
    bool skip_nil)
{
    int r;
    lua_getfield(mv->L, table_pos, key);
    if (skip_nil && lua_isnil(mv->L, -1)) {
        r = 0;
        goto done;
    }
    if (moon_visit_checktype_at(mv, key, -1, LUA_TBOOLEAN) < 0) {
        r = -1;
        goto done;
    }

    *p = lua_toboolean(mv->L, -1);
    r = 1;

done:
    lua_pop(mv->L, 1);
    return r;
}

int moon_visit_table_f(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    int (*f)(MoonVisit *mv, void *ud, int kpos, int vpos),
    void *ud,
    bool skip_nil)
{
    int r;

    lua_getfield(mv->L, table_pos, key);

    if (skip_nil && lua_isnil(mv->L, -1)) {
        r = 0;
        goto done;
    }

    r = moon_visit_table_f_at(mv, key, -1, f, ud);

done:
    lua_pop(mv->L, 1);
    return r;
}

int moon_visit_table_f_at(
    MoonVisit *mv,
    const char *what,
    int pos,
    int (*f)(MoonVisit *mv, void *ud, int kpos, int vpos),
    void *ud)
{
    int top = lua_gettop(mv->L);
    int r;

    if (moon_visit_checktype_at(mv, what, pos, LUA_TTABLE) < 0) {
        r = -1;
        goto done;
    }

    r = 0;
    int adj_pos = pos < 0 ? (pos - 1) : pos;
    const char *old_where = mv->where;
    lua_pushnil(mv->L);
    while (lua_next(mv->L, adj_pos)) {
        r = f(mv, ud, -2, -1);
        mv->where = old_where;
        if (r < 0) {
            lua_settop(mv->L, top);
            break;
        }
        lua_pop(mv->L, 1);
    }
done:
    return r;
}

int moon_visit_sint(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    int64_t *p,
    bool skip_nil)
{
    double d;
    int r = moon_visit_num(mv, table_pos, key, &d, skip_nil);
    if (r > 0) {
        if (isnan(d)) {
            moon_visit_err(mv, "%s: is NaN", key);
            return -1;
        }
        if (!(d >= -9223372036854775808.0 && d < 9223372036854775808.0)) {
            moon_visit_err(mv, "%s: value out of range INT64_MIN...INT64_MAX", key);
            return -1;
        }
        *p = d;
    }
    return r;
}

int moon_visit_uint(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    uint64_t *p,
    bool skip_nil)
{
    double d;
    int r = moon_visit_num(mv, table_pos, key, &d, skip_nil);
    if (r > 0) {
        if (isnan(d)) {
            moon_visit_err(mv, "%s: is NaN", key);
            return -1;
        }
        if (!(d >= 0.0 && d < 18446744073709551616.0)) {
            moon_visit_err(mv, "%s: value out of range 0...UINT64_MAX", key);
            return -1;
        }
        *p = d;
    }
    return r;
}

int moon_visit_scrutinize_table(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    bool nil_ok)
{
    lua_getfield(mv->L, table_pos, key);
    int type = lua_type(mv->L, -1);
    if (type == LUA_TNIL && nil_ok) {
        return 0;
    }
    if (moon_visit_checktype_at(mv, key, -1, LUA_TTABLE) < 0) {
        lua_pop(mv->L, 1);
        return -1;
    }
    return 0;
}

int moon_visit_scrutinize_str(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    const char **out,
    size_t *out_len,
    bool nil_ok)
{
    lua_getfield(mv->L, table_pos, key);
    int type = lua_type(mv->L, -1);
    if (type == LUA_TNIL && nil_ok) {
        *out = NULL;
        if (out_len) {
            *out_len = 0;
        }
        return 0;
    }

    if (moon_visit_checktype_at(mv, key, -1, LUA_TSTRING) < 0) {
        lua_pop(mv->L, 1);
        return -1;
    }

    if (out_len) {
        *out = lua_tolstring(mv->L, -1, out_len);
    } else {
        *out = lua_tostring(mv->L, -1);
    }
    return 0;
}
