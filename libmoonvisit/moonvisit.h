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

#ifndef moonvisit_h_
#define moonvisit_h_

#include <lua.h>
#include <stddef.h>
#include <stdbool.h>

#if ! defined(MOON_VISIT_PRINTF_ATTR)
# if __GNUC__ >= 2
#  define MOON_VISIT_PRINTF_ATTR(N, M) __attribute__((format(printf, N, M)))
# else
#  define MOON_VISIT_PRINTF_ATTR(N, M) /*nothing*/
# endif
#endif

typedef struct {
    lua_State *L;
    char *errbuf;
    size_t nerrbuf;
    const char *where;
} MoonVisit;

MOON_VISIT_PRINTF_ATTR(2, 3)
bool moon_visit_err(MoonVisit *mv, const char *fmt, ...);

int moon_visit_checktype_at(
    MoonVisit *mv,
    const char *what,
    int pos,
    int type);

int moon_visit_str_f(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    int (*f)(MoonVisit *mv, void *ud, const char *s, size_t ns),
    void *ud,
    bool skip_nil);

int moon_visit_str(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    char **ps,
    size_t *pn,
    bool skip_nil);

int moon_visit_num(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    double *p,
    bool skip_nil);

int moon_visit_bool(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    bool *p,
    bool skip_nil);

int moon_visit_table_f(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    int (*f)(MoonVisit *mv, void *ud, int kpos, int vpos),
    void *ud,
    bool skip_nil);

int moon_visit_table_f_at(
    MoonVisit *mv,
    const char *what,
    int pos,
    int (*f)(MoonVisit *mv, void *ud, int kpos, int vpos),
    void *ud);

#endif
