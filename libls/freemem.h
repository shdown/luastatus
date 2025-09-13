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

#ifndef ls_freemem_h_
#define ls_freemem_h_

#include "alloc_utils.h"

// This function is called whenever a dynamic array is cleared.
// We want to "preserve" at most 1 Kb of the previous capacity.
#define LS_FREEMEM(Data_, Size_, Capacity_) \
    do { \
        enum { SZ_  = sizeof(*(Data_))       }; \
        enum { CAP_ = 1024 / (SZ_ ? SZ_ : 1) }; \
        if ((Capacity_) > CAP_) { \
            (Data_) = ls_xrealloc((Data_), CAP_, SZ_); \
            (Capacity_) = CAP_; \
        } \
        (Size_) = 0; \
    } while (0)

#endif
