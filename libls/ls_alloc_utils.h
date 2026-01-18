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

#ifndef ls_alloc_utils_h_
#define ls_alloc_utils_h_

#include <stddef.h>
#include <string.h>

#include "ls_panic.h"
#include "ls_compdep.h"
#include "ls_assert.h"

#define LS_XNEW(Type_, NElems_) \
    ((Type_ *) ls_xmalloc(NElems_, sizeof(Type_)))

#define LS_XNEW0(Type_, NElems_) \
    ((Type_ *) ls_xcalloc(NElems_, sizeof(Type_)))

#define LS_M_X2REALLOC(Ptr_, NElemsPtr_) \
    ((__typeof__(Ptr_)) ls_x2realloc((Ptr_), (NElemsPtr_), sizeof((Ptr_)[0])))

#define LS_M_XMEMDUP(Ptr_, N_) \
    ((__typeof__(Ptr_)) ls_xmemdup((Ptr_), ((size_t) (N_)) * sizeof((Ptr_)[0])))

#define LS_M_XREALLOC(Ptr_, N_) \
    ((__typeof__(Ptr_)) ls_xrealloc((Ptr_), (N_), sizeof((Ptr_)[0])))

// Out-of-memory handler; should be called when an allocation fails.
//
// Should not return.
#define ls_oom() LS_PANIC("out of memory")

// The behavior is same as calling
//     /malloc(nelems * elemsz)/,
// except when the multiplication overflows, or the allocation fails. In these cases, this function
// panics.
LS_ATTR_WARN_UNUSED
void *ls_xmalloc(size_t nelems, size_t elemsz);

// The behavior is same as calling
//     /calloc(nelems, elemsz)/,
// except when the allocation fails. In that case, this function panics.
LS_ATTR_WARN_UNUSED
void *ls_xcalloc(size_t nelems, size_t elemsz);

// The behavior is same as calling
//     /realloc(p, nelems * elemsz)/,
// except when the multiplication overflows, or the reallocation fails. In these cases, this
// function panics.
//
// Zero /nelems/ and/or /elemsz/ are supported (even though C23 declared /realloc/ to zero size
// undefined behavior): if the total size requested is zero, this function /free/s the pointer
// and returns NULL.
LS_ATTR_WARN_UNUSED
void *ls_xrealloc(void *p, size_t nelems, size_t elemsz);

// The behavior is same as calling
//     /realloc(p, (*pnelems = F(*pnelems)) * elemsz)/,
// where F(n) = max(1, 2 * n),
// except when a multiplication overflows, or the reallocation fails. In these cases, this function
// panics.
//
// Zero /elemsz/ is NOT supported: it isn't clear what the semantics of this function should be in
// this case: technically you can store any number of zero-sized elements in an allocation of any
// size, but /*pnelems/ is limited to /SIZE_MAX/ (should we alter /*pnelems/ at all? If yes, what
// should we do in case of an overflow?). Also, C99 says there can't be any zero-sized types: empty
// structs or unions, and arrays of size 0, are prohibited.
LS_ATTR_WARN_UNUSED
void *ls_x2realloc(void *p, size_t *pnelems, size_t elemsz);

// Duplicates (as if with /malloc/) /n/ bytes of memory at address /p/. Panics on failure.
LS_ATTR_WARN_UNUSED
void *ls_xmemdup(const void *p, size_t n);

// The behavior is same as calling
//     /strdup(s)/,
// except when the allocation fails. In that case, this function panics.
LS_INHEADER LS_ATTR_WARN_UNUSED
char *ls_xstrdup(const char *s)
{
    LS_ASSERT(s != NULL);

    return ls_xmemdup(s, strlen(s) + 1);
}

#endif
