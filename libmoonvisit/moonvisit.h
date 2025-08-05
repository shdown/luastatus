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
#include <stdint.h>

#if ! defined(MOON_VISIT_PRINTF_ATTR)
# if __GNUC__ >= 2
#  define MOON_VISIT_PRINTF_ATTR(N, M) __attribute__((format(printf, N, M)))
# else
#  define MOON_VISIT_PRINTF_ATTR(N, M) /*nothing*/
# endif
#endif

typedef struct {
    // Filled by user, never altered (as a pointer) by library.
    lua_State *L;

    // Filled by user, never altered (as a pointer) by library.
    //
    // On error, this buffer is written to, assuming it has the size of 'nerrbuf', and is always
    // zero-terminated (unless 'nerrbuf' is 0, of course).
    char *errbuf;

    // Filled by the user, never altered by library.
    size_t nerrbuf;

    // Filled by user, but might be altered (as a pointer) by the library in the process of
    // saving/restoring old value of 'where'.
    const char *where;
} MoonVisit;

// Prints a formatted string into 'mv->errbuf' (prepended with "<where>: " if 'mv->where' is not
// NULL).
//
// Returns true on success, false on "encoding error".
MOON_VISIT_PRINTF_ATTR(2, 3)
bool moon_visit_err(MoonVisit *mv, const char *fmt, ...);

// Checks that the element at stack position 'pos' in 'mv->L' has type of 'type'.
// The fact that 'pos' is a valid stack index is *not* checked, simply assumed to be true.
//
// 'type' must be one of LUA_T* constants.
//
// 'what' must be string description of the value that is being tested.
//
// On success, returns 0.
//
// On failure, returns -1 and writes the error message into 'mv->errbuf'.
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

// Duplicates the string as if with 'malloc()', writing zero-terminated duplicated string into
// '*ps'; and, if 'pn' is not NULL, the length into '*pn'.
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

// Beware: current implementation can only faithfully (without precision loss) fetch integers that
// fit into 'double' (on virtually every platform it means integers with absolute value <= 2^53).
int moon_visit_sint(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    int64_t *p,
    bool skip_nil);

// Beware: current implementation can only faithfully (without precision loss) fetch integers that
// fit into 'double' (on virtually every platform it means values that are <= 2^53).
int moon_visit_uint(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    uint64_t *p,
    bool skip_nil);

// Pushes /t[key]/ on the stack, where /t/ is the table residing at stack position /table_pos/.
// Checks that the result is a table.
// If /nil_ok/ is true and the result is nil, pushes a nil instead of a table.
int moon_visit_scrutinize_table(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    bool nil_ok);

// Pushes /t[key]/ on the stack, where /t/ is the table residing at stack position /table_pos/.
//
// Checks that the result is a string. If so, /*out/ is set to the contents of the string,
// and, if /out_len != NULL/, writes the length of the string into /*out_len/.
//
// If /nil_ok/ is true and the result is nil, pushes a nil instead of a table, sets /*out/ to
// NULL, and, if /out_len != NULL/, sets /*out_len/ to zero.
int moon_visit_scrutinize_str(
    MoonVisit *mv,
    int table_pos,
    const char *key,
    const char **out,
    size_t *out_len,
    bool nil_ok);

#endif
