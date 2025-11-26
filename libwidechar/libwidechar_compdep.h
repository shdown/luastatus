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

#ifndef libwidechar_compdep_h_
#define libwidechar_compdep_h_

#if __GNUC__ >= 2
#   define LIBWIDECHAR_UNUSED __attribute__((unused))
#else
#   define LIBWIDECHAR_UNUSED /*nothing*/
#endif

#define LIBWIDECHAR_INHEADER static inline LIBWIDECHAR_UNUSED

#endif
