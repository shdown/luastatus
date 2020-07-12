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

#ifndef ls_tls_ebuf_h_
#define ls_tls_ebuf_h_

#include "compdep.h"
#include "cstring_utils.h"

enum { LS_TLS_EBUF_N = 512 };

char *ls_tls_ebuf(void);

LS_INHEADER const char *ls_tls_strerror(int errnum)
{
    return ls_strerror_r(errnum, ls_tls_ebuf(), LS_TLS_EBUF_N);
}

#endif
