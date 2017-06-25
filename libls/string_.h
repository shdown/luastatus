#ifndef ls_string_h_
#define ls_string_h_

#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include "vector.h"
#include "compdep.h"

// LSString is a string defined as an LS_VECTOR of chars. It is guaranteed to be defined like that,
// so that all the LS_VECTOR_* macros work with LSStrings.
typedef LS_VECTOR_OF(char) LSString;

// Assigns a string value to an LSString. If nbuf is 0, buf is not required to be dereferencable.
LS_INHEADER
void
ls_string_assign_b(LSString *s, const char *buf, size_t nbuf)
{
    LS_VECTOR_ENSURE(*s, nbuf);
    // see ../DOCS/empty-ranges-and-c-stdlib.md
    if (nbuf) {
        memcpy(s->data, buf, nbuf);
    }
    s->size = nbuf;
}

// Assigns a zero-terminated value to an LSString.
LS_INHEADER
void
ls_string_assign_s(LSString *s, const char *cstr)
{
    ls_string_assign_b(s, cstr, strlen(cstr));
}

// Appends a string to an LSString. If nbuf is 0, buf is not required to be dereferencable.
LS_INHEADER
void
ls_string_append_b(LSString *s, const char *buf, size_t nbuf)
{
    LS_VECTOR_ENSURE(*s, s->size + nbuf);
    // see ../DOCS/empty-ranges-and-c-stdlib.md
    if (nbuf) {
        memcpy(s->data + s->size, buf, nbuf);
    }
    s->size += nbuf;
}

// Appends a zero-terminated string to an LSString.
LS_INHEADER
void
ls_string_append_s(LSString *s, const char *cstr)
{
    ls_string_append_b(s, cstr, strlen(cstr));
}

// Appends a char to an LSString.
LS_INHEADER
void
ls_string_append_c(LSString *s, char c)
{
    LS_VECTOR_PUSH(*s, c);
}

// Assigns a char value to an LSString.
LS_INHEADER
void
ls_string_assign_c(LSString *s, char c)
{
    ls_string_assign_b(s, &c, 1);
}

// Appends a formatted string to an LSString.
// Returns false if an encoding error occurs.
bool
ls_string_append_vf(LSString *s, const char *fmt, va_list vl);

// Appends a formatted string to an LSString.
// Returns false if an encoding error occurs.
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

// Assigns a formatted string value to an LSString.
// Returns false if an encoding error occurs.
LS_INHEADER
bool
ls_string_assign_vf(LSString *s, const char *fmt, va_list vl)
{
    LS_VECTOR_CLEAR(*s);
    return ls_string_append_vf(s, fmt, vl);
}

// Assigns a formatted string value to an LSString.
// Returns false if an encoding error occurs.
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

#endif
