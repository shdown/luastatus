/*
 * Copyright (C) 2015-2020  luastatus developers
 *
 * This file is part of luastatus.
 *
 * luastatus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * luastatus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with luastatus.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "rules_names.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xatom.h>

#include "libls/algo.h"

static const char *NAMES_PROP_ATOM = "_XKB_RULES_NAMES";
static const long NAMES_PROP_MAXLEN = 1024;

static
const char *
maybe_advance(const char **pcur, const char *end)
{
    const char *cur = *pcur;
    if (cur == end) {
        return NULL;
    }
    *pcur += strlen(cur) + 1;
    return cur;
}

bool
rules_names_load(Display *dpy, RulesNames *out)
{
    *out = (RulesNames) {.data_ = NULL};

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

    const char *cur = (const char *) out->data_;
    const char *const end = cur + ndata;

    out->rules = maybe_advance(&cur, end);
    out->model = maybe_advance(&cur, end);
    out->layout = maybe_advance(&cur, end);
    out->options = maybe_advance(&cur, end);

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
