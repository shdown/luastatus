#include "load_by_name.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "luastatus/barlib.h"
#include "luastatus/plugin.h"
#include "libls/alloc_utils.h"
#include "libls/sprintf_utils.h"

bool
load_plugin_by_name(Plugin *p, const char *name)
{
    char *filename = strchr(name, '/')
        ? ls_xstrdup(name)
        : ls_xasprintf("%s/%s.so", LUASTATUS_PLUGINS_DIR, name);
    bool r = plugin_load(p, filename, name);
    free(filename);
    return r;
}

bool
load_barlib_by_name(Barlib *b, const char *name)
{
    char *filename = strchr(name, '/')
        ? ls_xstrdup(name)
        : ls_xasprintf("%s/%s.so", LUASTATUS_BARLIBS_DIR, name);
    bool r = barlib_load(b, filename);
    free(filename);
    return r;
}
