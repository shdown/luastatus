#ifndef taints_h_
#define taints_h_

#include <stddef.h>
#include <stdbool.h>
#include "barlib.h"
#include "widget.h"

bool
check_taints(Barlib *barlib, Widget *widgets, size_t nwidgets);

#endif
