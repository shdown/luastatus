#ifndef ls_alloc_utils_h_
#define ls_alloc_utils_h_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "panic.h"
#include "compdep.h"

// The behaviour is same as casting the result of
//     /ls_xmalloc(NElems_, sizeof(Type_))/
// to a pointer to /Type_/.
#define LS_XNEW(Type_, NElems_)  ((Type_ *) ls_xmalloc(NElems_, sizeof(Type_)))

// The behaviour is same as casting the result of
//     /ls_xcalloc(NElems_, sizeof(Type_))/
// to a pointer to /Type_/.
#define LS_XNEW0(Type_, NElems_) ((Type_ *) ls_xcalloc(NElems_, sizeof(Type_)))

// Out-of-memory handler; should be called when an allocation fails.
//
// Should not return.
#define ls_oom() LS_PANIC("out of memory")

// The behaviour is same as calling
//     /malloc(nelems * elemsz)/,
// except when the multiplication overflows, or the allocation fails. In these cases, this function
// panics.
LS_INHEADER
void *
ls_xmalloc(size_t nelems, size_t elemsz)
{
    if (elemsz && nelems > SIZE_MAX / elemsz) {
        goto oom;
    }
    const size_t n = nelems * elemsz;
    void *r = malloc(n);
    if (n && !r) {
        goto oom;
    }
    return r;

oom:
    ls_oom();
}

// The behaviour is same as calling
//     /calloc(nelems, elemsz)/,
// except when the allocation fails. In that case, this function panics.
LS_INHEADER
void *
ls_xcalloc(size_t nelems, size_t elemsz)
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
LS_INHEADER
void *
ls_xrealloc(void *p, size_t nelems, size_t elemsz)
{
    if (elemsz && nelems > SIZE_MAX / elemsz) {
        goto oom;
    }
    const size_t n = nelems * elemsz;
    void *r = realloc(p, n);
    if (n && !r) {
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
LS_INHEADER
void *
ls_x2realloc(void *p, size_t *pnelems, size_t elemsz)
{
    const size_t oldnelems = *pnelems;

    size_t newnelems;
    size_t n;

    if (oldnelems) {
        if (elemsz && oldnelems > SIZE_MAX / 2 / elemsz) {
            goto oom;
        }
        newnelems = oldnelems * 2;
        n = newnelems * elemsz;
    } else {
        newnelems = 1;
        n = elemsz;
    }

    void *r = realloc(p, n);
    if (n && !r) {
        goto oom;
    }
    *pnelems = newnelems;
    return r;

oom:
    ls_oom();
}

// Duplicates (as if with /malloc/) /n/ bytes of memory at address /p/. Panics on failure.
LS_INHEADER
void *
ls_xmemdup(const void *p, size_t n)
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
LS_INHEADER
char *
ls_xstrdup(const char *s)
{
    char *r = strdup(s);
    if (!r) {
        ls_oom();
    }
    return r;
}

#endif
