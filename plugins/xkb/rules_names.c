#include "rules_names.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xatom.h>

static const char *NAMES_PROP_ATOM = "_XKB_RULES_NAMES";
static const long NAMES_PROP_MAXLEN = 1024;

bool
rules_names_load(Display *dpy, RulesNames *out)
{
    out->rules = NULL;
    out->model = NULL;
    out->layout = NULL;
    out->options = NULL;
    out->data_ = NULL;

    bool ret = false;

    Atom rules_atom = XInternAtom(dpy, NAMES_PROP_ATOM, True);
    if (rules_atom == None) {
        goto done;
    }
    Atom actual_type;
    int fmt;
    unsigned long ndata, bytes_after;
    if (XGetWindowProperty(dpy, DefaultRootWindow(dpy), rules_atom, 0L, NAMES_PROP_MAXLEN,
                           False, XA_STRING, &actual_type, &fmt, &ndata, &bytes_after, &out->data_)
        != Success)
    {
        goto done;
    }
    if (bytes_after || actual_type != XA_STRING || fmt != 8) {
        goto done;
    }

    const char *ptr = (const char *) out->data_;
    const char *end = (const char *) out->data_ + ndata;

    if (ptr != end) {
        out->rules = ptr;
        ptr += strlen(ptr) + 1;
    }
    if (ptr != end) {
        out->model = ptr;
        ptr += strlen(ptr) + 1;
    }
    if (ptr != end) {
        out->layout = ptr;
        ptr += strlen(ptr) + 1;
    }
    if (ptr != end) {
        out->options = ptr;
        ptr += strlen(ptr) + 1;
    }

    ret = true;

done:
    if (!ret) {
        rules_names_destroy(out);
    }
    return ret;
}

void
rules_names_destroy(const RulesNames *rn)
{
    if (rn->data_) {
        XFree(rn->data_);
    }
}
