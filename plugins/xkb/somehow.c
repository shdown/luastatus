/*
 * Copyright (C) 2021-2025  luastatus developers
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
#include <stdlib.h>
#include "libls/ls_alloc_utils.h"
#include "libsafe/safev.h"
#include "parse_symbols.h"

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

static inline bool bad_matches(SAFEV bad, SAFEV v)
{
    if (!SAFEV_starts_with(v, bad)) {
        return false;
    }
    size_t nbad = SAFEV_len(bad);
    if (nbad == SAFEV_len(v)) {
        return true;
    }
    char c = SAFEV_at(v, nbad);
    return c == '(' || c == ':';
}

static inline bool check_bad(SAFEV bad, SAFEV v)
{
    if (!SAFEV_len(v)) {
        return true;
    }
    if (!SAFEV_len(bad)) {
        return false;
    }
    for (;;) {
        size_t i = SAFEV_index_of(bad, ',');
        if (i == (size_t) -1) {
            return bad_matches(bad, v);
        }
        if (bad_matches(SAFEV_subspan(bad, 0, i), v)) {
            return true;
        }
        bad = SAFEV_suffix(bad, i + 1);
    }
}

typedef struct {
    LS_StringArray *out;
    SAFEV bad_v;
} MyParseUserdata;

static void my_parse_cb(void *vud, SAFEV segment)
{
    MyParseUserdata *ud = vud;
    if (!check_bad(ud->bad_v, segment)) {
        ls_strarr_append(ud->out, SAFEV_ptr_UNSAFE(segment), SAFEV_len(segment));
    }
}

void somehow_parse_symbols(const char *symbols, LS_StringArray *out, const char *bad)
{
    SAFEV symbols_v = SAFEV_new_from_cstr_UNSAFE(symbols);
    MyParseUserdata ud = {
        .out = out,
        .bad_v = SAFEV_new_from_cstr_UNSAFE(bad),
    };
    parse_symbols(symbols_v, my_parse_cb, &ud);
}
