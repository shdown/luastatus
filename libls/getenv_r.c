#include "getenv_r.h"

#include <stddef.h>
#include <string.h>

const char *
ls_getenv_r(const char *name, char *const *env)
{
    if ((strchr(name, '='))) {
        return NULL;
    }
    size_t nname = strlen(name);
    for (char *const *s = env; *s; ++s) {
        const char *entry = *s;
        if (strncmp(entry, name, nname) == 0 && entry[nname] == '=') {
            return entry + nname + 1;
        }
    }
    return NULL;
}
