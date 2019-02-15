#include "getenv_r.h"

#include <stddef.h>
#include <string.h>

extern char **environ;

const char *
ls_getenv_r(const char *name)
{
    if ((strchr(name, '='))) {
        return NULL;
    }
    const size_t nname = strlen(name);
    for (char **s = environ; *s; ++s) {
        const char *entry = *s;
        if (strncmp(entry, name, nname) == 0 && entry[nname] == '=') {
            return entry + nname + 1;
        }
    }
    return NULL;
}
