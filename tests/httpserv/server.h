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

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef int (*BeforeRequestCallback)(
    uint64_t cookie,
    const char *path,
    bool is_method_post,
    char **out_mime_type,
    void *ud);

typedef char *(*WriteBodyCallback)(
    uint64_t cookie,
    const char *body,
    size_t nbody,
    size_t *out_len,
    void *ud);

typedef void (*AfterRequestCallback)(
    void *ud);

typedef void (*ReadyCallback)(
    void *ud);

bool run_server(
    int port,
    BeforeRequestCallback before_req_cb,
    WriteBodyCallback write_body_cb,
    AfterRequestCallback after_req_cb,
    ReadyCallback ready_cb,
    void *ud
);
