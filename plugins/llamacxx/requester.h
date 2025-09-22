/*
 * Copyright (C) 2025  luastatus developers
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

#ifndef requester_h_
#define requester_h_

#include <stdbool.h>
#include "libls/ls_string.h"
#include "external_context.h"
#include "my_error.h"
#include "priv.h"

struct Requester;
typedef struct Requester Requester;

bool requester_global_init(MyError *out_err);

Requester *requester_new(
    const PrivCnxSettings *settings,
    ExternalContext ectx,
    MyError *out_err);

bool requester_make_request(
    Requester *R,
    const char *extra_params_json,
    const char *prompt,
    LS_String *out,
    MyError *out_err);

void requester_destroy(Requester *R);

#endif
