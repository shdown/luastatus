#ifndef load_by_name_h_
#define load_by_name_h_

#include <stdbool.h>
#include "plugin.h"
#include "barlib.h"

bool
load_plugin_by_name(Plugin *p, const char *name);

bool
load_barlib_by_name(Barlib *b, const char *name);

#endif
