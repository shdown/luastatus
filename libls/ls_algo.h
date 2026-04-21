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

#define LS_ARRAY_SIZE(X_) (sizeof(X_) / sizeof((X_)[0]))

#define LS_SWAP(Type_, X_, Y_) \
    do { \
        Type_ ls_swap_tmp_ = (X_); \
        (X_) = (Y_); \
        (Y_) = ls_swap_tmp_; \
    } while (0)
