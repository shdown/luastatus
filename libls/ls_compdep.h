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

#ifndef ls_compdep_h_
#define ls_compdep_h_

#include <stdlib.h> // for abort()

// ------------------------------------------------------------
// GCC version (set by clang to some very old version)
// ------------------------------------------------------------

#define LS_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

// ------------------------------------------------------------
// Clang stuff
// ------------------------------------------------------------

#if defined(__has_attribute)
#   define LS_CLANG_HAS_ATTRIBUTE(X_) __has_attribute(X_)
#else
#   define LS_CLANG_HAS_ATTRIBUTE(X_) 0
#endif

#if defined(__has_builtin)
#   define LS_CLANG_HAS_BUILTIN(X_) __has_builtin(X_)
#else
#   define LS_CLANG_HAS_BUILTIN(X_) 0
#endif

// ------------------------------------------------------------
// LS_TRAP()
// ------------------------------------------------------------
#if LS_GCC_VERSION >= 40204 || LS_CLANG_HAS_BUILTIN(__builtin_trap)
#   define LS_TRAP() __builtin_trap()
#else
#   define LS_TRAP() abort()
#endif

// ------------------------------------------------------------
// Attributes
// ------------------------------------------------------------

#if __GNUC__ >= 2
#   define LS_ATTR_UNUSED           __attribute__((unused))
#   define LS_ATTR_PRINTF(N_, M_)   __attribute__((format(printf, N_, M_)))
#   define LS_ATTR_NORETURN         __attribute__((noreturn))
#else
#   define LS_ATTR_UNUSED           /*nothing*/
#   define LS_ATTR_PRINTF(N_, M_)   /*nothing*/
#   define LS_ATTR_NORETURN         /*nothing*/
#endif

#if LS_GCC_VERSION >= 30406 || LS_CLANG_HAS_ATTRIBUTE(warn_unused_result)
#   define LS_ATTR_WARN_UNUSED __attribute__((warn_unused_result))
#else
#   define LS_ATTR_WARN_UNUSED /*nothing*/
#endif

// ------------------------------------------------------------
// LS_INHEADER
// ------------------------------------------------------------
#define LS_INHEADER static inline LS_ATTR_UNUSED

#endif
