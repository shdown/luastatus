#include "sepstate.h"

#include <lauxlib.h>
#include <lualib.h>
#include <pthread.h>
#include "libls/alloc_utils.h"
#include "pth_check.h"
#include "lua_libs.h"

SepState global_sepstate = {.L = NULL};

void
sepstate_ensure_inited(void)
{
    if (global_sepstate.L) {
        // already initialized
        return;
    }
    if (!(global_sepstate.L = luaL_newstate())) {
        ls_oom();
    }
    luaL_openlibs(global_sepstate.L);
    lualibs_inject(global_sepstate.L);
    PTH_CHECK(pthread_mutex_init(&global_sepstate.lua_mtx, NULL));
}
