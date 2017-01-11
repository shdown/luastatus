#ifndef ls_strarr_h_
#define ls_strarr_h_

#include <stddef.h>
#include "string.h"
#include "vector.h"

typedef struct {
    LSString _buf;
    LS_VECTOR_OF(size_t) _offsets;
} LSStringArray;

#define LS_STRARR_INITIALIZER {._buf = LS_VECTOR_NEW(), ._offsets = LS_VECTOR_NEW()}

LS_INHEADER
void
ls_strarr_append(LSStringArray *sa, const char *buf, size_t nbuf)
{
    LS_VECTOR_PUSH(sa->_offsets, sa->_buf.size);
    ls_string_append_b(&sa->_buf, buf, nbuf);
}

LS_INHEADER
size_t
ls_strarr_size(LSStringArray sa)
{
    return sa._offsets.size;
}

LS_INHEADER
const char *
ls_strarr_at(LSStringArray sa, size_t index, size_t *n)
{
    const size_t begin = sa._offsets.data[index];
    const size_t end = index + 1 == sa._offsets.size
                       ? sa._buf.size
                       : sa._offsets.data[index + 1];
    if (n) {
        *n = end - begin;
    }
    return sa._buf.data + begin;
}

LS_INHEADER
void
ls_strarr_clear(LSStringArray *sa)
{
    LS_VECTOR_CLEAR(sa->_buf);
    LS_VECTOR_CLEAR(sa->_offsets);
}

LS_INHEADER
void
ls_strarr_destroy(LSStringArray sa)
{
    LS_VECTOR_FREE(sa._buf);
    LS_VECTOR_FREE(sa._offsets);
}

#endif
