#include "rules_names.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xatom.h>

#include "libls/algo.h"

static const char *NAMES_PROP_ATOM = "_XKB_RULES_NAMES";
static const long NAMES_PROP_MAXLEN = 1024;

bool
rules_names_load(Display *dpy, RulesNames *out)
{
    *out = (RulesNames) {0};

    Atom rules_atom = XInternAtom(dpy, NAMES_PROP_ATOM, True);
    if (rules_atom == None) {
        goto error;
    }
    Atom actual_type;
    int fmt;
    unsigned long ndata, bytes_after;
    if (XGetWindowProperty(dpy, DefaultRootWindow(dpy), rules_atom, 0L, NAMES_PROP_MAXLEN,
                           False, XA_STRING, &actual_type, &fmt, &ndata, &bytes_after, &out->data_)
        != Success)
    {
        goto error;
    }
    if (bytes_after || actual_type != XA_STRING || fmt != 8) {
        goto error;
    }

    const char *ptr = (const char *) out->data_;
    const char *const end = ptr + ndata;

    const char **table[] = {
        &out->rules,
        &out->model,
        &out->layout,
        &out->options,
    };
    for (size_t i = 0;
         i < LS_ARRAY_SIZE(table) && ptr != end;
         ++i, ptr += strlen(ptr) + 1)
    {
        *table[i] = ptr;
    }

    return true;

error:
    rules_names_destroy(out);
    return false;
}

void
rules_names_destroy(RulesNames *rn)
{
    if (rn->data_) {
        XFree(rn->data_);
    }
}
