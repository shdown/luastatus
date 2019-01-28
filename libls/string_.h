#ifndef ls_string_h_
#define ls_string_h_

#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "vector.h"
#include "compdep.h"

typedef LS_VECTOR_OF(char) LSString;

LS_INHEADER
void
ls_string_assign_b(LSString *s, const char *buf, size_t nbuf)
{
    LS_VECTOR_ENSURE(*s, nbuf);
    // see DOCS/c_notes/empty-ranges-and-c-stdlib.md
    if (nbuf) {
        memcpy(s->data, buf, nbuf);
    }
    s->size = nbuf;
}

LS_INHEADER
void
ls_string_assign_s(LSString *s, const char *cstr)
{
    ls_string_assign_b(s, cstr, strlen(cstr));
}

LS_INHEADER
void
ls_string_assign_c(LSString *s, char c)
{
    ls_string_assign_b(s, &c, 1);
}

LS_INHEADER
void
ls_string_append_b(LSString *s, const char *buf, size_t nbuf)
{
    LS_VECTOR_ENSURE(*s, s->size + nbuf);
    // see DOCS/c_notes/empty-ranges-and-c-stdlib.md
    if (nbuf) {
        memcpy(s->data + s->size, buf, nbuf);
    }
    s->size += nbuf;
}

LS_INHEADER
void
ls_string_append_s(LSString *s, const char *cstr)
{
    ls_string_append_b(s, cstr, strlen(cstr));
}

LS_INHEADER
void
ls_string_append_c(LSString *s, char c)
{
    LS_VECTOR_PUSH(*s, c);
}

// Appends a formatted string to /s/. Returns /true/ on success or /false/ if an encoding error
// occurs.
bool
ls_string_append_vf(LSString *s, const char *fmt, va_list vl);

// Appends a formatted string to /s/. Returns /true/ on success or /false/ if an encoding error
// occurs.
LS_INHEADER LS_ATTR_PRINTF(2, 3)
bool
ls_string_append_f(LSString *s, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    bool r = ls_string_append_vf(s, fmt, vl);
    va_end(vl);
    return r;
}

// Returns /false/ if an encoding error occurs.
LS_INHEADER
bool
ls_string_assign_vf(LSString *s, const char *fmt, va_list vl)
{
    LS_VECTOR_CLEAR(*s);
    return ls_string_append_vf(s, fmt, vl);
}

// Returns /false/ if an encoding error occurs.
LS_INHEADER LS_ATTR_PRINTF(2, 3)
bool
ls_string_assign_f(LSString *s, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    bool r = ls_string_assign_vf(s, fmt, vl);
    va_end(vl);
    return r;
}

LS_INHEADER
LSString
ls_string_new_from_s(const char *cstr)
{
    LSString r = LS_VECTOR_NEW();
    ls_string_assign_s(&r, cstr);
    return r;
}

LS_INHEADER
LSString
ls_string_new_from_b(const char *buf, size_t nbuf)
{
    LSString r = LS_VECTOR_NEW();
    ls_string_assign_b(&r, buf, nbuf);
    return r;
}

LS_INHEADER
LSString
ls_string_new_from_c(char c)
{
    LSString r = LS_VECTOR_NEW();
    LS_VECTOR_PUSH(r, c);
    return r;
}

// If an encoding error occurs:
//   * if /NDEBUG/ is not defined, it aborts;
//   * if /DDEBUG/ is defined, the behaviour is undefined.
LS_INHEADER
LSString
ls_string_new_from_vf(const char *fmt, va_list vl)
{
    LSString s = LS_VECTOR_NEW();
    bool r = ls_string_append_vf(&s, fmt, vl);
    (void) r;
    assert(r);
    return s;
}

// If an encoding error occurs:
//   * if /NDEBUG/ is not defined, it aborts;
//   * if /DDEBUG/ is defined, the behaviour is undefined.
LS_INHEADER LS_ATTR_PRINTF(1, 2)
LSString
ls_string_new_from_f(const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    LSString s = ls_string_new_from_vf(fmt, vl);
    va_end(vl);
    return s;
}

LS_INHEADER
bool
ls_string_eq(LSString a, LSString b)
{
    // We have to check that the size is not zero before calling /memcmp/:
    // see DOCS/c_notes/empty-ranges-and-c-stdlib.md
    return a.size == b.size && (a.size == 0 || memcmp(a.data, b.data, a.size) == 0);
}

// Swaps two string efficiently (in O(1) time).
LS_INHEADER
void
ls_string_swap(LSString *a, LSString *b)
{
    LSString tmp = *a;
    *a = *b;
    *b = tmp;
}

#endif
