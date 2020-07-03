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

#ifndef ls_alloc_utils_h_
#define ls_alloc_utils_h_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "panic.h"
#include "compdep.h"

#define LS_XNEW(Type_, NElems_)  ((Type_ *) ls_xmalloc(NElems_, sizeof(Type_)))

#define LS_XNEW0(Type_, NElems_) ((Type_ *) ls_xcalloc(NElems_, sizeof(Type_)))

// Out-of-memory handler; should be called when an allocation fails.
//
// Should not return.
#define ls_oom() LS_PANIC("out of memory")

// The behaviour is same as calling
//     /malloc(nelems * elemsz)/,
// except when the multiplication overflows, or the allocation fails. In these cases, this function
// panics.
LS_INHEADER void *ls_xmalloc(size_t nelems, size_t elemsz)
{
    if (elemsz && nelems > SIZE_MAX / elemsz) {
        goto oom;
    }
    void *r = malloc(nelems * elemsz);
    if (nelems && elemsz && !r) {
        goto oom;
    }
    return r;

oom:
    ls_oom();
}

// The behaviour is same as calling
//     /calloc(nelems, elemsz)/,
// except when the allocation fails. In that case, this function panics.
LS_INHEADER void *ls_xcalloc(size_t nelems, size_t elemsz)
{
    void *r = calloc(nelems, elemsz);
    if (nelems && elemsz && !r) {
        ls_oom();
    }
    return r;
}

// The behaviour is same as calling
//     /realloc(p, nelems * elemsz)/,
// except when the multiplication overflows, or the reallocation fails. In these cases, this
// function panics.
LS_INHEADER void *ls_xrealloc(void *p, size_t nelems, size_t elemsz)
{
    if (elemsz && nelems > SIZE_MAX / elemsz) {
        goto oom;
    }
    void *r = realloc(p, nelems * elemsz);
    if (nelems && elemsz && !r) {
        goto oom;
    }
    return r;

oom:
    ls_oom();
}

// The behaviour is same as calling
//     /realloc(p, (*pnelems = F(*pnelems)) * elemsz)/,
// where F(n) = max(1, 2 * n),
// except when a multiplication overflows, or the reallocation fails. In these cases, this function
// panics.
LS_INHEADER void *ls_x2realloc(void *p, size_t *pnelems, size_t elemsz)
{
    size_t new_nelems;
    if (*pnelems) {
        if (elemsz && *pnelems > SIZE_MAX / 2 / elemsz) {
            goto oom;
        }
        new_nelems = *pnelems * 2;
    } else {
        new_nelems = 1;
    }

    void *r = realloc(p, new_nelems * elemsz);
    if (elemsz && !r) {
        goto oom;
    }
    *pnelems = new_nelems;
    return r;

oom:
    ls_oom();
}

// Duplicates (as if with /malloc/) /n/ bytes of memory at address /p/. Panics on failure.
LS_INHEADER void *ls_xmemdup(const void *p, size_t n)
{
    void *r = malloc(n);
    if (n) {
        if (!r) {
            ls_oom();
        }
        memcpy(r, p, n);
    }
    return r;
}

// The behaviour is same as calling
//     /strdup(s)/,
// except when the allocation fails. In that case, this function panics.
LS_INHEADER char *ls_xstrdup(const char *s)
{
    return ls_xmemdup(s, strlen(s) + 1);
}

#endif
