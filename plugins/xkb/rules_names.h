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

#ifndef rules_names_h_
#define rules_names_h_

#include <X11/Xlib.h>
#include <stdbool.h>

typedef struct {
    // These all are either zero-terminated strings allocated as if with /malloc()/, or /NULL/.
    char *rules;
    char *model;
    char *layout;
    char *options;
} RulesNames;

bool rules_names_load(Display *dpy, RulesNames *out);

void rules_names_free(RulesNames rn);

#endif
