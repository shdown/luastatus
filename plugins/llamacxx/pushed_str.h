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

#ifndef pushed_str_h_
#define pushed_str_h_

#include <pthread.h>
#include <lua.h>

typedef struct {
    char *val;
    pthread_mutex_t mtx;
} PushedStr;

void pushed_str_new(PushedStr *x);

void pushed_str_push_luafunc(PushedStr *x, lua_State *L);

char *pushed_str_fetch(PushedStr *x);

void pushed_str_destroy(PushedStr *x);

#endif
