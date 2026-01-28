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

#include "prop_field.h"
#include <string.h>
#include <stddef.h>
#include "libls/ls_assert.h"

char **lookup_in_pfields(PField *fields, const char *key)
{
    LS_ASSERT(key != NULL);

    for (PField *f = fields; f->key; ++f) {
        if (strcmp(f->key, key) == 0) {
            return &f->value;
        }
    }
    return NULL;
}
