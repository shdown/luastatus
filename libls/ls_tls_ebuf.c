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

#include "ls_tls_ebuf.h"

#include <pthread.h>
#include <stdlib.h>
#include "ls_alloc_utils.h"
#include "ls_panic.h"

static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void mk_key(void)
{
    LS_PTH_CHECK(pthread_key_create(&key, free));
}

char *ls_tls_ebuf(void)
{
    pthread_once(&key_once, mk_key);
    char *p = pthread_getspecific(key);
    if (!p) {
        p = LS_XNEW(char, LS_TLS_EBUF_N);
        LS_PTH_CHECK(pthread_setspecific(key, p));
    }
    return p;
}
