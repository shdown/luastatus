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

#ifndef ls_sig_utils_h_
#define ls_sig_utils_h_

#include <signal.h>
#include <stdio.h>

#include "compdep.h"

// Like /sigfillset/, but panics on failure.
LS_INHEADER
void
ls_xsigfillset(sigset_t *set)
{
    if (sigfillset(set) < 0) {
        fputs("luastatus: sigfillset() failed.\n", stderr);
        abort();
    }
}

// Like /sigemptyset/, but panics on failure.
LS_INHEADER
void
ls_xsigemptyset(sigset_t *set)
{
    if (sigemptyset(set) < 0) {
        fputs("luastatus: sigemptyset() failed.\n", stderr);
        abort();
    }
}

#endif
