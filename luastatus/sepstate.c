#include "sepstate.h"

#include <lauxlib.h>
#include <lualib.h>
#include <pthread.h>
#include "libls/alloc_utils.h"
#include "pth_check.h"
#include "lua_libs.h"

SepState sepstate = {.L = NULL};

void
sepstate_ensure_inited(void)
{
    if (sepstate.L) {
        // already initialized
        return;
    }
    if (!(sepstate.L = luaL_newstate())) {
        ls_oom();
    }
    luaL_openlibs(sepstate.L);
    lualibs_inject(sepstate.L);
    PTH_CHECK(pthread_mutex_init(&sepstate.lua_mtx, NULL));
}
