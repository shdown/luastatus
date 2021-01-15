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

// All 'visit' functions (whose names do *not* end with "_at") operate on a *field* of the table at
// stack position 'table_pos' in 'mv->L', at key 'key'. The fact that the value at stack position
// 'table_pos' is a table is *not* checked, simply assumed to be true.
//
// If 'skip_nil' is true and the value is nil, no action is performed, and 0 is returned.
//
// On error, the error message is written into 'mv->errbuf', and -1 is returned.
//
// Otherwise, the return value depends on whether the function is "_f"-type (accepting a "visitor"
// function pointer 'f') or not:
//
// * If it *is* "_f"-type, then the return value of 'f' is returned. If 'f' might be called multiple
// times, then the return value of the last call is returned, but if 'f' returns negative value, no
// further calls are made. If 'f' has not been called at all, 0 is returned.
//
// * If it is *not* "_f"-type, then 1 is returned, indicating that the value was *not* nil and has
// been written to the output pointer.

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

#endif
