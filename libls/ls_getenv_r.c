/*
 * Copyright (C) 2015-2025  luastatus developers
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

#include "ls_getenv_r.h"

#include <stddef.h>
#include <string.h>

extern char **environ;

const char *ls_getenv_r(const char *name)
{
    if (!environ) {
        return NULL;
    }
    if ((strchr(name, '='))) {
        return NULL;
    }
    size_t nname = strlen(name);
    for (char **s = environ; *s; ++s) {
        const char *entry = *s;
        if (strncmp(entry, name, nname) == 0 && entry[nname] == '=') {
            return entry + nname + 1;
        }
    }
    return NULL;
}
