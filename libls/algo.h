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

#ifndef ls_algo_h_
#define ls_algo_h_

#include <stdbool.h>

#include "compdep.h"

#define LS_ARRAY_SIZE(X_) (sizeof(X_) / sizeof((X_)[0]))

// Checks if /x/:
//   * is not NaN;
//   * belongs to /[lbound, ubound]/.
LS_INHEADER
bool
ls_is_between_d(double x, double lbound, double ubound)
{
    return x >= lbound && x <= ubound;
}

#endif
