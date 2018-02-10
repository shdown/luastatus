#ifndef ls_alloc_utils_h_
#define ls_alloc_utils_h_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "compdep.h"

// The behaviour is same as casting the result of
//     /ls_xmalloc(NElems_, sizeof(Type_))/
// to a pointer to /Type_/.
#define LS_XNEW(Type_, NElems_)  ((Type_ *) ls_xmalloc(NElems_, sizeof(Type_)))

// The behaviour is same as casting the result of
//     /ls_xcalloc(NElems_, sizeof(Type_))/
// to a pointer to /Type_/.
#define LS_XNEW0(Type_, NElems_) ((Type_ *) ls_xcalloc(NElems_, sizeof(Type_)))

// Out-of-memory handler, called when allocation fails.
//
// Should not return.
LS_ATTR_NORETURN
void
ls_oom(void);

// The behaviour is same as calling
//     /malloc(nelems * elemsz)/,
// except when the multiplication overflows, or the allocation fails. In these cases, this function
// panics.
LS_INHEADER LS_ATTR_MALLOC LS_ATTR_ALLOC_SIZE2(1, 2)
void *
ls_xmalloc(size_t nelems, size_t elemsz)
{
    size_t n;
#if LS_HAS_BUILTIN_OVERFLOW(__builtin_mul_overflow)
    if (__builtin_mul_overflow(nelems, elemsz, &n)) {
        goto oom;
    }
#else
    if (elemsz && nelems > SIZE_MAX / elemsz) {
        goto oom;
    }
    n = nelems * elemsz;
#endif
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
LS_INHEADER LS_ATTR_MALLOC LS_ATTR_ALLOC_SIZE2(1, 2)
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
LS_INHEADER LS_ATTR_ALLOC_SIZE2(2, 3)
void *
ls_xrealloc(void *p, size_t nelems, size_t elemsz)
{
    size_t n;
#if LS_HAS_BUILTIN_OVERFLOW(__builtin_mul_overflow)
    if (__builtin_mul_overflow(nelems, elemsz, &n)) {
        goto oom;
    }
#else
    if (elemsz && nelems > SIZE_MAX / elemsz) {
        goto oom;
    }
    n = nelems * elemsz;
#endif
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
// where F(n) = max(1, 2*n),
// except when the multiplication overflows, or the reallocation fails. In these cases, this
// function panics.
LS_INHEADER
void *
ls_x2realloc(void *p, size_t *pnelems, size_t elemsz)
{
    const size_t oldnelems = *pnelems;

    size_t newnelems;
    size_t n;

    if (oldnelems) {
#if LS_HAS_BUILTIN_OVERFLOW(__builtin_mul_overflow)
        if (__builtin_mul_overflow(oldnelems, 2, &newnelems)) {
            goto oom;
        }
        if (__builtin_mul_overflow(newnelems, elemsz, &n)) {
            goto oom;
        }
#else
        if (elemsz && oldnelems > SIZE_MAX / 2 / elemsz) {
            goto oom;
        }
        newnelems = oldnelems * 2;
        n = newnelems * elemsz;
#endif
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
LS_INHEADER LS_ATTR_MALLOC
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
LS_INHEADER LS_ATTR_MALLOC
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
