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

#include "comm.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "libls/ls_alloc_utils.h"
#include "libls/ls_panic.h"

static pthread_mutex_t mtx;

void comm_global_init(void)
{
    LS_PTH_CHECK(pthread_mutex_init(&mtx, NULL));
}

void comm_lock(void)
{
    LS_PTH_CHECK(pthread_mutex_lock(&mtx));
}

void comm_unlock(void)
{
    LS_PTH_CHECK(pthread_mutex_unlock(&mtx));
}

void comm_set(Comm *c, const char *new_data, size_t new_data_len)
{
    free(c->data);
    c->data = ls_xmemdup(new_data, new_data_len);
    c->data_len = new_data_len;
}

int comm_cas(
    Comm *c,
    const char *old_data, size_t old_data_len,
    const char *new_data, size_t new_data_len)
{
    if (c->data_len != old_data_len) {
        return 0;
    }
    if (old_data_len && memcmp(c->data, old_data, old_data_len) != 0) {
        return 0;
    }
    comm_set(c, new_data, new_data_len);
    return 1;
}

void comm_destroy(Comm *c)
{
    free(c->data);
}

void comm_global_deinit(void)
{
    LS_PTH_CHECK(pthread_mutex_destroy(&mtx));
}
