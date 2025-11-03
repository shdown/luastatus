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

#ifndef ls_panic_h_
#define ls_panic_h_

#include <stdlib.h>
#include "ls_compdep.h"

// Note: external /ls_panic_*/ functions do not abort because this way the
// compiler wouldn't know they do not return (and thus the code after the macro
// call is unreachable); this would result in warnings in, e.g., functions
// returning non-void that call /LS_PANIC()/ or /LS_PANIC_WITH_ERRNUM()/ as the
// last statement in a final block.
//
// We also don't want to introduce another macro in "ls_compdep.h" for
// /__attribute__((noreturn))/.
//
// So, instead, the macros themselves call /abort()/.

// Logs /Msg_/ and aborts.
#define LS_PANIC(Msg_) \
    do { \
        ls_panic__log(Msg_, __FILE__, __LINE__); \
        abort(); \
    } while (0)

// Logs /Msg_/ with errno value /Errnum_/ and aborts.
#define LS_PANIC_WITH_ERRNUM(Msg_, Errnum_) \
    do { \
        ls_panic_with_errnum__log(Msg_, Errnum_, __FILE__, __LINE__); \
        abort(); \
    } while (0)

// Asserts that a /pthread_*/ call was successful.
#define LS_PTH_CHECK(Expr_) ls_pth_check__impl(Expr_, #Expr_, __FILE__, __LINE__)

// Internal function. Normally should not be called manually.
void ls_panic__log(const char *msg, const char *file, int line);

// Internal function. Normally should not be called manually.
void ls_panic_with_errnum__log(const char *msg, int errnum, const char *file, int line);

// Internal function. Normally should not be called manually.
void ls_pth_check__log_and_abort(int ret, const char *expr, const char *file, int line);

// Implementation part for /LS_PTH_CHECK()/. Normally should not be called manually.
LS_INHEADER void ls_pth_check__impl(int ret, const char *expr, const char *file, int line)
{
    if (ret != 0) {
        ls_pth_check__log_and_abort(ret, expr, file, line);
    }
}

#endif
