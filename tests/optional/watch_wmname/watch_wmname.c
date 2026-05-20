/*
 * Copyright (C) 2026  luastatus developers
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

#include <stdio.h>
#include <stddef.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

static inline void say(const char *s)
{
    puts(s);
    fflush(stdout);
}

static void query_and_say(Display *dpy, Window root)
{
    char *name;
    if (!XFetchName(dpy, root, &name)) {
        goto fail;
    }
    if (!name) {
        goto fail;
    }
    say(name);
    XFree(name);
    return;

fail:
    say("");
}

int main(int argc, char **argv)
{
    const char *dpyname;
    if (argc <= 1) {
        dpyname = NULL;
    } else if (argc == 2) {
        dpyname = argv[1];
    } else {
        fprintf(stderr, "USAGE: watch_wmname [DISPLAY_NAME]\n");
        return 2;
    }

    Display *dpy = XOpenDisplay(dpyname);
    if (!dpy) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    Window root = DefaultRootWindow(dpy);
    Atom wm_name_atom = XInternAtom(dpy, "WM_NAME", False);

    XSelectInput(dpy, root, PropertyChangeMask);

    query_and_say(dpy, root);

    for (;;) {
        XEvent ev;
        XNextEvent(dpy, &ev);

        if (ev.type == PropertyNotify &&
            ev.xproperty.window == root &&
            ev.xproperty.atom == wm_name_atom)
        {
            query_and_say(dpy, root);
        }
    }
}
