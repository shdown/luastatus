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

#pragma once

#include "ls_compdep.h"

LS_ATTR_NORETURN void ls__do_panic_impl__(
    const char *macro_name,
    const char *expr,
    const char *msg,
    const char *func, const char *file, int line);

LS_ATTR_NORETURN void ls__do_panic_with_errnum_impl__(
    const char *macro_name,
    const char *expr,
    const char *msg,
    int errnum,
    const char *func, const char *file, int line);

#define LS__DO_PANIC__(MacroName_, Expr_, Msg_) \
    ls__do_panic_impl__( \
        MacroName_, \
        Expr_, \
        Msg_, \
        __func__, __FILE__, __LINE__)

#define LS__DO_PANIC_WITH_ERRNUM__(MacroName_, Expr_, Msg_, Errnum_) \
    ls__do_panic_with_errnum_impl__( \
        MacroName_, \
        Expr_, \
        Msg_, \
        Errnum_, \
        __func__, __FILE__, __LINE__)

// Logs /Msg_/ and aborts.
#define LS_PANIC(Msg_) \
    LS__DO_PANIC__("LS_PANIC", "", Msg_)

// Logs /Msg_/ with errno value /Errnum_/ and aborts.
#define LS_PANIC_WITH_ERRNUM(Msg_, Errnum_) \
    LS__DO_PANIC_WITH_ERRNUM__("LS_PANIC_WITH_ERRNUM", "", Msg_, Errnum_)

// Asserts that a /pthread_*/ call was successful.
#define LS_PTH_CHECK(Expr_) \
    do { \
        int ls_pth_rc_ = (Expr_); \
        if (ls_pth_rc_ != 0) { \
            LS__DO_PANIC_WITH_ERRNUM__("LS_PTH_CHECK", #Expr_, "check failed", ls_pth_rc_); \
        } \
    } while (0)

// Asserts that /Expr_/ is true.
#define LS_ASSERT(Expr_) \
    do { \
        if (!(Expr_)) { \
            LS__DO_PANIC__("LS_ASSERT", #Expr_, "assertion failed"); \
        } \
    } while (0)

// Just use it to tell the compiler some code is unreachable.
#define LS_MUST_BE_UNREACHABLE() LS_PANIC("LS_MUST_BE_UNREACHABLE()")
