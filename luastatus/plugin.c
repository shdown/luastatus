#include "plugin.h"

#include <stdbool.h>
#include <lua.h>
#include <dlfcn.h>
#include <stdlib.h>
#include "include/loglevel.h"
#include "libls/alloc_utils.h"
#include "sane_dlerror.h"
#include "log.h"

bool
plugin_load(Plugin *p, const char *filename, const char *name)
{
    p->dlhandle = NULL;

    (void) dlerror(); // clear any last error
    if (!(p->dlhandle = dlopen(filename, RTLD_NOW))) {
        internal_logf(LUASTATUS_ERR, "dlopen: %s: %s", filename, sane_dlerror());
        goto error;
    }
    int *p_lua_ver = dlsym(p->dlhandle, "LUASTATUS_PLUGIN_LUA_VERSION_NUM");
    if (!p_lua_ver) {
        internal_logf(LUASTATUS_ERR, "dlsym: LUASTATUS_PLUGIN_LUA_VERSION_NUM: %s",
                      sane_dlerror());
        goto error;
    }
    if (*p_lua_ver != LUA_VERSION_NUM) {
        internal_logf(LUASTATUS_ERR,
                      "plugin '%s' was compiled with LUA_VERSION_NUM=%d and luastatus with %d",
                      filename, *p_lua_ver, LUA_VERSION_NUM);
        goto error;
    }
    LuastatusPluginIface *p_iface = dlsym(p->dlhandle, "luastatus_plugin_iface");
    if (!p_iface) {
        internal_logf(LUASTATUS_ERR, "dlsym: luastatus_plugin_iface: %s", sane_dlerror());
        goto error;
    }
    p->iface = *p_iface;

    p->name = ls_xstrdup(name);
    return true;

error:
    if (p->dlhandle) {
        dlclose(p->dlhandle);
    }
    return false;
}

void
plugin_unload(Plugin *p)
{
    free(p->name);
    dlclose(p->dlhandle);
}
