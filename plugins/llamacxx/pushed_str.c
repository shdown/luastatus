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

#include "pushed_str.h"
#include <pthread.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include "libls/ls_alloc_utils.h"
#include "libls/ls_panic.h"

//MLC_PUSH_SCOPE("PushedStr:init")
void pushed_str_new(PushedStr *x)
{
//MLC_INIT("val")
    x->val = NULL;
//MLC_INIT("mtx")
    LS_PTH_CHECK(pthread_mutex_init(&x->mtx, NULL));
}
//MLC_POP_SCOPE()

static inline char *xchg(PushedStr *x, char *new_val)
{
    LS_PTH_CHECK(pthread_mutex_lock(&x->mtx));

    char *old_val = x->val;
    x->val = new_val;

    LS_PTH_CHECK(pthread_mutex_unlock(&x->mtx));

    return old_val;
}

static int my_lfunc(lua_State *L)
{
    size_t ns;
    const char *s = luaL_checklstring(L, 1, &ns);
    if (ns != strlen(s)) {
        return luaL_error(L, "argument contains NUL character");
    }

    PushedStr *x = lua_touserdata(L, lua_upvalueindex(1));

    char *old_val = xchg(x, ls_xstrdup(s));
    free(old_val);

    return 0;
}

void pushed_str_push_luafunc(PushedStr *x, lua_State *L)
{
    lua_pushlightuserdata(L, x); // L: userdata
    lua_pushcclosure(L, my_lfunc, 1); // L: func
}

char *pushed_str_fetch(PushedStr *x)
{
    return xchg(x, NULL);
}

//MLC_PUSH_SCOPE("PushedStr:deinit")
void pushed_str_destroy(PushedStr *x)
{
//MLC_DEINIT("val")
    free(x->val);
//MLC_DEINIT("mtx")
    LS_PTH_CHECK(pthread_mutex_destroy(&x->mtx));
}
//MLC_POP_SCOPE()
