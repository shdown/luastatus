#ifndef ls_strarr_h_
#define ls_strarr_h_

#include <stddef.h>
#include "string_.h"
#include "vector.h"

// An array of constant strings on a single buffer. Panics on allocation failure.

typedef struct {
    LSString _buf;
    LS_VECTOR_OF(size_t) _offsets;
} LSStringArray;

// Creates an empty string array.
LS_INHEADER
LSStringArray
ls_strarr_new(void)
{
    return (LSStringArray) {
        ._buf = LS_VECTOR_NEW(),
        ._offsets = LS_VECTOR_NEW(),
    };
}

// Creates an empty string array with a space for nelems elements with total length of sumlen
// reserved.
LS_INHEADER
LSStringArray
ls_strarr_new_reserve(size_t sumlen, size_t nelems)
{
    return (LSStringArray) {
        ._buf = LS_VECTOR_NEW_RESERVE(char, sumlen),
        ._offsets = LS_VECTOR_NEW_RESERVE(size_t, nelems),
    };
}

// Appends a string to a string array.
LS_INHEADER
void
ls_strarr_append(LSStringArray *sa, const char *buf, size_t nbuf)
{
    LS_VECTOR_PUSH(sa->_offsets, sa->_buf.size);
    ls_string_append_b(&sa->_buf, buf, nbuf);
}

// Returns the size of a string array.
LS_INHEADER
size_t
ls_strarr_size(LSStringArray sa)
{
    return sa._offsets.size;
}

// Returns a pointer to the start of the string at specified index in a string array. In n is not a
// null pointer, *n is set to the size of that string.
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

// Clears a string array.
LS_INHEADER
void
ls_strarr_clear(LSStringArray *sa)
{
    LS_VECTOR_CLEAR(sa->_buf);
    LS_VECTOR_CLEAR(sa->_offsets);
}

// Shrinks a string array to its real size.
LS_INHEADER
void
ls_strarr_shrink(LSStringArray *sa)
{
    LS_VECTOR_SHRINK(sa->_buf);
    LS_VECTOR_SHRINK(sa->_offsets);
}

// Destroys a string array.
LS_INHEADER
void
ls_strarr_destroy(LSStringArray sa)
{
    LS_VECTOR_FREE(sa._buf);
    LS_VECTOR_FREE(sa._offsets);
}

#endif
