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

#ifndef libwidechar_xspan_h_
#define libwidechar_xspan_h_

#include <stddef.h>
#include <stdbool.h>
#include "libsafe/safev.h"
#include "libwidechar_compdep.h"

typedef struct {
    SAFEV v;
    size_t cur;
} xspan;

LIBWIDECHAR_INHEADER xspan v_to_xspan(SAFEV v)
{
    return (xspan) {
        .v  = v,
        .cur = 0,
    };
}

LIBWIDECHAR_INHEADER bool xspan_nonempty(xspan x)
{
    return x.cur != SAFEV_len(x.v);
}

LIBWIDECHAR_INHEADER void xspan_advance(xspan *x, size_t n)
{
    x->cur += n;
}

LIBWIDECHAR_INHEADER size_t xspan_processed_len(xspan x)
{
    return x.cur;
}

LIBWIDECHAR_INHEADER SAFEV xspan_processed_v(xspan x)
{
    return SAFEV_subspan(x.v, 0, x.cur);
}

LIBWIDECHAR_INHEADER SAFEV xspan_unprocessed_v(xspan x)
{
    return SAFEV_suffix(x.v, x.cur);
}

LIBWIDECHAR_INHEADER xspan xspan_skip_processed(xspan x)
{
    return (xspan) {
        .v = xspan_unprocessed_v(x),
        .cur = 0,
    };
}

#endif
