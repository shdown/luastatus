#include "string_set.h"

#include <string.h>
#include <stdlib.h>

static
int
elem_cmp(const void *a, const void *b)
{
    return strcmp(* (const char *const *) a,
                  * (const char *const *) b);
}

void
string_set_freeze(StringSet *s)
{
    if (s->size) {
        qsort(s->data, s->size, sizeof(char *), elem_cmp);
    }
}

bool
string_set_contains(StringSet s, const char *val)
{
    if (!s.size) {
        return false;
    }
    return bsearch(&val, s.data, s.size, sizeof(char *), elem_cmp) != NULL;
}
