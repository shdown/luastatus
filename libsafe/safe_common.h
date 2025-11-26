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

#ifndef libsafe_safe_common_h_
#define libsafe_safe_common_h_

#if __GNUC__ >= 2
#   define LIBSAFE_ATTR_UNUSED           __attribute__((unused))
#   define LIBSAFE_ATTR_NORETURN         __attribute__((noreturn))
#else
#   define LIBSAFE_ATTR_UNUSED           /*nothing*/
#   define LIBSAFE_ATTR_NORETURN         /*nothing*/
#endif

LIBSAFE_ATTR_NORETURN void libsafe_assert_failed__(
    const char *expr,
    const char *func,
    const char *file,
    int line);

#define LIBSAFE_INHEADER static inline LIBSAFE_ATTR_UNUSED

#define LIBSAFE_ASSERT(Expr_) \
    do { \
        if (!(Expr_)) { \
            libsafe_assert_failed__(#Expr_, __func__, __FILE__, __LINE__); \
        } \
    } while (0)

#endif
