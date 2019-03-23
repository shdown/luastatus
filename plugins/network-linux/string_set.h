#ifndef string_set_h_
#define string_set_h_

#include <stdlib.h>
#include <stdbool.h>

#include "libls/alloc_utils.h"
#include "libls/compdep.h"
#include "libls/vector.h"

typedef LS_VECTOR_OF(char *) StringSet;

LS_INHEADER
StringSet
string_set_new()
{
    return (StringSet) LS_VECTOR_NEW();
}

// Clears and unfreezes the set.
LS_INHEADER
void
string_set_reset(StringSet *s)
{
    for (size_t i = 0; i < s->size; ++i) {
        free(s->data[i]);
    }
    LS_VECTOR_CLEAR(*s);
}

LS_INHEADER
void
string_set_add(StringSet *s, const char *val)
{
    LS_VECTOR_PUSH(*s, ls_xstrdup(val));
}

void
string_set_freeze(StringSet *s);

bool
string_set_contains(StringSet s, const char *val);

LS_INHEADER
void
string_set_destroy(StringSet s)
{
    for (size_t i = 0; i < s.size; ++i) {
        free(s.data[i]);
    }
    LS_VECTOR_FREE(s);
}

#endif
