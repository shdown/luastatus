/*
 * Copyright (C) 2015-2021  luastatus developers
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

#include "wrongly.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <X11/X.h>
#include <X11/Xatom.h>

#include "libls/alloc_utils.h"

static const char *NAMES_PROP_ATOM = "_XKB_RULES_NAMES";

static inline char *dup_and_advance(const char **pcur, const char *end)
{
    const char *cur = *pcur;
    if (cur == end)
        return NULL;

    size_t n = strlen(cur);
    *pcur += n + 1;
    return ls_xmemdup(cur, n + 1);
}

bool wrongly_fetch(Display *dpy, WronglyResult *out)
{
    bool ret = false;
    unsigned char *data = NULL;

    Atom rules_atom = XInternAtom(dpy, NAMES_PROP_ATOM, True);
    if (rules_atom == None)
        goto done;

    unsigned long ndata;
    long maxlen = 1024;
    for (;;) {
        Atom actual_type;
        int fmt;
        unsigned long bytes_after;
        if (XGetWindowProperty(
                    dpy,
                    DefaultRootWindow(dpy),
                    rules_atom,
                    0L,
                    maxlen,
                    False,
                    XA_STRING,
                    &actual_type,
                    &fmt,
                    &ndata,
                    &bytes_after,
                    &data)
                != Success)
        {
            data = NULL;
            goto done;
        }
        if (actual_type != XA_STRING)
            goto done;
        if (fmt != 8)
            goto done;

        if (!bytes_after)
            break;

        if (maxlen > LONG_MAX / 2)
            goto done;
        maxlen *= 2;
    }

    const char *cur = (const char *) data;
    const char *end = cur + ndata;

    out->rules = dup_and_advance(&cur, end);
    out->model = dup_and_advance(&cur, end);
    out->layout = dup_and_advance(&cur, end);
    out->options = dup_and_advance(&cur, end);

    ret = true;
done:
    if (data)
        XFree(data);
    return ret;
}

void wrongly_parse_layout(const char *layout, LS_StringArray *out)
{
    const char *prev = layout;
    size_t balance = 0;
    for (;; ++layout) {
        switch (*layout) {
        case '(':
            ++balance;
            break;
        case ')':
            --balance;
            break;
        case ',':
            if (balance == 0) {
                ls_strarr_append(out, prev, layout - prev);
                prev = layout + 1;
            }
            break;
        case '\0':
            ls_strarr_append(out, prev, layout - prev);
            return;
        }
    }
}
