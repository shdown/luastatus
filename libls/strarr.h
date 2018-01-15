#ifndef ls_strarr_h_
#define ls_strarr_h_

#include <stddef.h>

#include "string_.h"
#include "vector.h"
#include "compdep.h"

// An array of constant strings on a single buffer. Panics on allocation failure.

typedef struct {
    LSString buf_;
    LS_VECTOR_OF(size_t) offsets_;
} LSStringArray;

// Creates an empty string array.
LS_INHEADER
LSStringArray
ls_strarr_new(void)
{
    return (LSStringArray) {
        .buf_ = LS_VECTOR_NEW(),
        .offsets_ = LS_VECTOR_NEW(),
    };
}

// Creates an empty string array with a space for /nelems/ elements with total length of /totlen/
// reserved.
LS_INHEADER
LSStringArray
ls_strarr_new_reserve(size_t totlen, size_t nelems)
{
    return (LSStringArray) {
        .buf_ = LS_VECTOR_NEW_RESERVE(char, totlen),
        .offsets_ = LS_VECTOR_NEW_RESERVE(size_t, nelems),
    };
}

// Appends a buffer /buf/ of size /nbuf/ to a string array to a string array /sa/.
LS_INHEADER
void
ls_strarr_append(LSStringArray *sa, const char *buf, size_t nbuf)
{
    LS_VECTOR_PUSH(sa->offsets_, sa->buf_.size);
    ls_string_append_b(&sa->buf_, buf, nbuf);
}

// Returns the size of a string array /sa/.
LS_INHEADER
size_t
ls_strarr_size(LSStringArray sa)
{
    return sa.offsets_.size;
}

// Returns a pointer to the start of the string at specified index in a string array /sa/. If /n/ is
// not /NULL/, /*n/ is set to the size of that string.
LS_INHEADER
const char *
ls_strarr_at(LSStringArray sa, size_t index, size_t *n)
{
    const size_t begin = sa.offsets_.data[index];
    const size_t end = index + 1 == sa.offsets_.size
                       ? sa.buf_.size
                       : sa.offsets_.data[index + 1];
    if (n) {
        *n = end - begin;
    }
    return sa.buf_.data + begin;
}

// Clears a string array /sa/.
LS_INHEADER
void
ls_strarr_clear(LSStringArray *sa)
{
    LS_VECTOR_CLEAR(sa->buf_);
    LS_VECTOR_CLEAR(sa->offsets_);
}

// Shrinks a string array /sa/ to its real size.
LS_INHEADER
void
ls_strarr_shrink(LSStringArray *sa)
{
    LS_VECTOR_SHRINK(sa->buf_);
    LS_VECTOR_SHRINK(sa->offsets_);
}

// Destroys a (previously initialized) string array /sa/.
LS_INHEADER
void
ls_strarr_destroy(LSStringArray sa)
{
    LS_VECTOR_FREE(sa.buf_);
    LS_VECTOR_FREE(sa.offsets_);
}

#endif
