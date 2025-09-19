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

#ifndef ls_xallocf_h_
#define ls_xallocf_h_

#include <stdarg.h>
#include "ls_compdep.h"

char *ls_xallocvf(const char *fmt, va_list vl);

LS_INHEADER LS_ATTR_PRINTF(1, 2)
char *ls_xallocf(const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    char *r = ls_xallocvf(fmt, vl);
    va_end(vl);
    return r;
}

#endif
