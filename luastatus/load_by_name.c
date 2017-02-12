#include "load_by_name.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "luastatus/barlib.h"
#include "luastatus/plugin.h"
#include "libls/alloc_utils.h"
#include "libls/sprintf_utils.h"

// http://insanecoding.blogspot.ru/2007/11/pathmax-simply-isnt.html
// PATH_MAX is not only useless, but also is, according to POSIX, optional:
//
// http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/limits.h.html
//
// > A definition of one of the symbolic names in the following list shall be omitted from
// > <limits.h> on specific implementations where the corresponding value is equal to or greater
// > than the stated minimum, but is unspecified.
//
// We don't care about performance here, so we just allocate filenames on the heap.

bool
load_plugin_by_name(Plugin *p, const char *name)
{
    if ((strchr(name, '/'))) {
        return plugin_load(p, name, name);
    } else {
        char *filename = ls_xasprintf("%s/%s.so", LUASTATUS_PLUGINS_DIR, name);
        bool r = plugin_load(p, filename, name);
        free(filename);
        return r;
    }
}

bool
load_barlib_by_name(Barlib *b, const char *name)
{
    if ((strchr(name, '/'))) {
        return barlib_load(b, name);
    } else {
        char *filename = ls_xasprintf("%s/%s.so", LUASTATUS_BARLIBS_DIR, name);
        bool r = barlib_load(b, filename);
        free(filename);
        return r;
    }
}
