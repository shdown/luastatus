#ifndef ls_string_h_
#define ls_string_h_

#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include "vector.h"
#include "compdep.h"

typedef LS_VECTOR_OF(char) LSString;

LS_INHEADER
void
ls_string_assign_b(LSString *s, const char *buf, size_t nbuf)
{
    LS_VECTOR_ENSURE(*s, nbuf);
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
ls_string_append_b(LSString *s, const char *buf, size_t nbuf)
{
    LS_VECTOR_ENSURE(*s, s->size + nbuf);
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

LS_INHEADER
void
ls_string_assign_c(LSString *s, char c)
{
    ls_string_assign_b(s, &c, 1);
}

bool
ls_string_append_vf(LSString *s, const char *fmt, va_list vl);

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

LS_INHEADER
bool
ls_string_assign_vf(LSString *s, const char *fmt, va_list vl)
{
    LS_VECTOR_CLEAR(*s);
    return ls_string_append_vf(s, fmt, vl);
}

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
