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

#ifndef luastatus_comm_h_
#define luastatus_comm_h_

#include <stddef.h>

void comm_global_init(void);

typedef struct {
    char *data;
    size_t data_len;
} Comm;

#define COMM_INITIALIZER {0}

void comm_lock(void);

void comm_unlock(void);

void comm_set(Comm *c, const char *new_data, size_t new_data_len);

int comm_cas(
    Comm *c,
    const char *old_data, size_t old_data_len,
    const char *new_data, size_t new_data_len);

void comm_destroy(Comm *c);

void comm_global_deinit(void);

#endif
