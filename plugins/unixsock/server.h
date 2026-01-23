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

#ifndef server_h_
#define server_h_

#include <stddef.h>
#include <stdbool.h>
#include <poll.h>

#include "libls/ls_time_utils.h"

enum {
    SERVER_MAX_CLIENTS_LIMIT = 1024 * 1024 * 1024,
};

struct Server;
typedef struct Server Server;

Server *server_new(
        int fd,
        size_t max_clients);

int server_poll(
        Server *S,
        LS_TimeDelta timeout,
        struct pollfd **out,
        size_t *out_n,
        bool *out_can_accept);

int server_read_from_client(
        Server *S,
        size_t idx);

const char *server_get_full_line(
        Server *S,
        size_t idx,
        size_t *out_len);

void server_drop_client(
        Server *S,
        size_t idx);

// After this call, pollfd's returned from /server_poll/ are invalidated; it is
// invalid to use them anymore.
int server_accept_new_client(Server *S);

void server_compact(Server *S);

void server_destroy(Server *S);

#endif
