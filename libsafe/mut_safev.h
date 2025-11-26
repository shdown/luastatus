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

#ifndef libsafe_mut_safev_h_
#define libsafe_mut_safev_h_

#include "safe_common.h"
#include <stddef.h>
#include <string.h>
#include "safev.h"

typedef struct {
    char *mut_s__;
    size_t mut_n__;
} MUT_SAFEV;

LIBSAFE_INHEADER MUT_SAFEV MUT_SAFEV_new_empty(void)
{
    return (MUT_SAFEV) {.mut_s__ = NULL, .mut_n__ = 0};
}

LIBSAFE_INHEADER MUT_SAFEV MUT_SAFEV_new_UNSAFE(char *buf, size_t nbuf)
{
    return (MUT_SAFEV) {
        .mut_s__ = buf,
        .mut_n__ = nbuf,
    };
}

#define MUT_SAVEF_STATIC_INIT_UNSAFE(Ptr_, Len_) \
    { \
        .mut_s__ = (Ptr_), \
        .mut_n__ = (Len_), \
    }

#define MUT_SAFEV_set_at(MutV_, I_, Ch_) \
    do { \
        MUT_SAFEV MVmv__ = (MutV_); \
        size_t MVi__ = (I_); \
        char MVch__ = (Ch_); \
        LIBSAFE_ASSERT(MVi__ < MVmv__.mut_n__); \
        MVmv__.mut_s__[MVi__] = MVch__; \
    } while (0)

LIBSAFE_INHEADER SAFEV MUT_SAFEV_TO_SAFEV(MUT_SAFEV Mv)
{
    return SAFEV_new_UNSAFE(Mv.mut_s__, Mv.mut_n__);
}

#endif
