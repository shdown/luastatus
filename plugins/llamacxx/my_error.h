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

#ifndef my_error_h_
#define my_error_h_

#include <stdlib.h>
#include <stdarg.h>
#include "libls/ls_compdep.h"
#include "libls/ls_xallocf.h"
#include "libls/ls_assert.h"

typedef struct {
    char meta_domain;
    int meta_code;
    char *descr;
} MyError;

LS_INHEADER void my_error_dispose(MyError *e)
{
    free(e->descr);
}

LS_INHEADER void my_error_clear(MyError *e)
{
    my_error_dispose(e);
    *e = (MyError) {0};
}

LS_INHEADER void my_error_printf(MyError *e, const char *fmt, ...)
{
    free(e->descr);

    va_list vl;
    va_start(vl, fmt);
    e->descr = ls_xallocvf(fmt, vl);
    va_end(vl);
}

LS_INHEADER void my_error_set_meta(MyError *e, char domain, int code)
{
    e->meta_domain = domain;
    e->meta_code = code;
}

LS_INHEADER const char *my_error_cstr(MyError *e)
{
    LS_ASSERT(e->descr != NULL);
    return e->descr;
}

#endif
