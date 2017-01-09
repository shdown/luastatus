#ifndef ls_alloc_utils_h_
#define ls_alloc_utils_h_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "compdep.h"

#define LS_XNEW(Type_, N_)  ((Type_*) ls_xmalloc(N_, sizeof(Type_)))

#define LS_XNEW0(Type_, N_) ((Type_*) ls_xcalloc(N_, sizeof(Type_)))

LS_ATTR_NORETURN
void
ls_oom(void);

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
    if (nelems > SIZE_MAX / elemsz) {
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

LS_INHEADER LS_ATTR_MALLOC LS_ATTR_ALLOC_SIZE2(1, 2)
void *
ls_xcalloc(size_t nelems, size_t elemsz)
{
    void *r = calloc(nelems, elemsz);
    if (nelems && !r) {
        ls_oom();
    }
    return r;
}

LS_INHEADER LS_ATTR_ALLOC_SIZE2(2, 3)
void *
ls_xrealloc(void *p, size_t nelems, size_t elemsz)
{
    if (nelems > SIZE_MAX / elemsz) {
        goto oom;
    }
    size_t n = nelems * elemsz;
    void *r = realloc(p, n);
    if (n && !r) {
        goto oom;
    }
    return r;

oom:
    ls_oom();
}

LS_INHEADER
void *
ls_x2realloc(void *p, size_t *pnelems, size_t elemsz)
{
    size_t oldnelems = *pnelems;
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
        if (oldnelems > SIZE_MAX / 2 / elemsz) {
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

LS_INHEADER LS_ATTR_MALLOC LS_ATTR_ALLOC_SIZE1(2)
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
