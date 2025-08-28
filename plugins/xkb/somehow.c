/*
 * Copyright (C) 2021  luastatus developers
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

#include "somehow.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/XKB.h>
#include <X11/extensions/XKBstr.h>
#include <X11/XKBlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "libls/alloc_utils.h"

char *somehow_fetch_symbols(Display *dpy, uint64_t deviceid)
{
    char *ret = NULL;

    XkbDescRec *k = XkbAllocKeyboard();
    if (!k)
        goto done;

    k->dpy = dpy;
    if (deviceid != XkbUseCoreKbd)
        k->device_spec = deviceid;

    XkbGetNames(dpy, XkbSymbolsNameMask, k);
    if (!k->names)
        goto done;

    Atom a = k->names->symbols;
    if (a == None)
        goto done;

    char *s = XGetAtomName(dpy, a);
    ret = ls_xstrdup(s);
    XFree(s);

done:
    if (k) {
        XkbFreeNames(k, XkbSymbolsNameMask, True);
        XFree(k);
    }
    return ret;
}

// Assumes that /s[nbad]/ and /bad[nbad]/ are defined and set to either the next symbol or '\0'.
static inline bool bad_matches(const char *bad, size_t nbad, const char *s, size_t ns)
{
    if (ns >= nbad && memcmp(bad, s, nbad) == 0) {
        if (ns == nbad)
            return true;

        char c = s[nbad];
        if (c == '(' || c == ':')
            return true;
    }
    return false;
}

static inline bool check_bad(const char *bad, const char *s, size_t ns)
{
    if (!ns)
        return true;
    if (bad[0] == '\0')
        return false;
    for (const char *bad_next; (bad_next = strchr(bad, ',')); bad = bad_next + 1)
        if (bad_matches(bad, bad_next - bad, s, ns))
            return true;
    return bad_matches(bad, strlen(bad), s, ns);
}

void somehow_parse_symbols(const char *symbols, LSStringArray *out, const char *bad)
{
    const char *prev = symbols;
    size_t balance = 0;
    for (;; ++symbols) {
        switch (*symbols) {
        case '(':
            ++balance;
            break;
        case ')':
            --balance;
            break;
        case '+':
            if (balance == 0) {
                if (!check_bad(bad, prev, symbols - prev)) {
                    ls_strarr_append(out, prev, symbols - prev);
                }
                prev = symbols + 1;
            }
            break;
        case '\0':
            if (!check_bad(bad, prev, symbols - prev)) {
                ls_strarr_append(out, prev, symbols - prev);
            }
            return;
        }
    }
}
