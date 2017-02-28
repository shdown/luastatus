#include "barlib.h"

#include <lua.h>
#include <dlfcn.h>
#include <assert.h>
#include <stdbool.h>
#include "libls/compdep.h"
#include "include/loglevel.h"
#include "sane_dlerror.h"
#include "log.h"
#include "pth_check.h"

// http://pubs.opengroup.org/onlinepubs/9699919799/functions/dlerror.html
//
// > It is implementation-defined whether or not the dlerror() function is thread-safe.
//
// So, barlib_load isn't thread-safe.
bool
barlib_load(Barlib *b, const char *filename)
{
    b->dlhandle = NULL;

    (void) dlerror(); // clear any last error
    if (!(b->dlhandle = dlopen(filename, RTLD_NOW))) {
        internal_logf(LUASTATUS_ERR, "dlopen: %s: %s", filename, sane_dlerror());
        goto error;
    }
    int *p_lua_ver = dlsym(b->dlhandle, "LUASTATUS_BARLIB_LUA_VERSION_NUM");
    if (!p_lua_ver) {
        internal_logf(LUASTATUS_ERR, "dlsym: LUASTATUS_BARLIB_LUA_VERSION_NUM: %s",
                      sane_dlerror());
        goto error;
    }
    if (*p_lua_ver != LUA_VERSION_NUM) {
        internal_logf(LUASTATUS_ERR,
                      "barlib '%s' was compiled with LUA_VERSION_NUM=%d and luastatus with %d",
                      filename, *p_lua_ver, LUA_VERSION_NUM);
        goto error;
    }
    LuastatusBarlibIface *p_iface = dlsym(b->dlhandle, "luastatus_barlib_iface");
    if (!p_iface) {
        internal_logf(LUASTATUS_ERR, "dlsym: luastatus_barlib_iface: %s", sane_dlerror());
        goto error;
    }
    b->iface = *p_iface;
    PTH_CHECK(pthread_mutex_init(&b->set_mtx, NULL));

    b->state = BARLIB_STATE_LOADED;
    return true;

error:
    if (b->dlhandle) {
        dlclose(b->dlhandle);
    }
    return false;
}

bool
barlib_init(Barlib *b, LuastatusBarlibData data, const char *const *opts, size_t nwidgets)
{
    assert(b->state == BARLIB_STATE_LOADED);

    b->data = data;
    switch (b->iface.init(&b->data, opts, nwidgets)) {
    case LUASTATUS_BARLIB_INIT_RESULT_OK:
        b->state = BARLIB_STATE_INITED;
        return true;
    case LUASTATUS_BARLIB_INIT_RESULT_ERR:
        internal_logf(LUASTATUS_ERR, "barlib's init() failed");
        return false;
    }
    LS_UNREACHABLE();
}

void
barlib_uninit(Barlib *b)
{
    assert(b->state == BARLIB_STATE_INITED);

    b->iface.destroy(&b->data);
    b->state = BARLIB_STATE_LOADED;
}

void
barlib_unload(Barlib *b)
{
    switch (b->state) {
    case BARLIB_STATE_INITED:
        barlib_uninit(b);
        /* fallthru */
    case BARLIB_STATE_LOADED:
        dlclose(b->dlhandle);
        PTH_CHECK(pthread_mutex_destroy(&b->set_mtx));
    }
}
