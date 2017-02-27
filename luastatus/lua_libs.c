#include "lua_libs.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <lauxlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <lua.h>
#include "libls/lua_utils.h"
#include "libls/getenv_r.h"
#include "libls/alloc_utils.h"
#include "libls/vector.h"
#include "libls/string.h"
#include "libls/io_utils.h"
#include "libls/sprintf_utils.h"
#include "libls/errno_utils.h"
#include "libls/osdep.h"
#include "libls/io_utils.h"
#include "include/loglevel.h"
#include "log.h"
#include "config.h"
#include "barlib.h"
#include "widget.h"

static
int
l_os_execute(lua_State *L)
{
    return luaL_error(L, "os.execute isn't thread-safe; use luastatus.rc or luastatus.spawn "
                         "instead");
}

static
int
l_os_exit(lua_State *L)
{
    return luaL_error(L, "os.exit isn't thread-safe; don't use it");
}

static
int
l_os_setlocale(lua_State *L)
{
    return luaL_error(L, "os.setlocale would affect all threads and probably break something; "
                         "don't use it");
}

static
int
l_os_getenv(lua_State *L)
{
    const char *r = ls_getenv_r(luaL_checkstring(L, 1));
    if (r) {
        lua_pushstring(L, r);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static
int
get_rc(int status)
{
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    } else {
        return 128;
    }
}

static
size_t
dollar_len(const char *s, size_t ns)
{
    while (ns && s[ns - 1] == '\n') {
        --ns;
    }
    return ns;
}

static
int
lspawn_func(lua_State *L, const char *funcname, bool return_output, bool return_exitcode)
{
    int init_top = lua_gettop(L);
    LS_VECTOR_OF(char *) argv = LS_VECTOR_NEW_RESERVE(char *, 8);
    LSString output_buf = LS_VECTOR_NEW();
    int pipe_fd = -1;
    pid_t pid = -1;
    bool luaerr = false;
    char luaerr_msg[1024];

#define USAGE_ERR(...) \
    do { \
        ls_ssnprintf(luaerr_msg, sizeof(luaerr_msg), __VA_ARGS__); \
        luaerr = true; \
        goto cleanup; \
    } while (0)

#define SPAWN_ERR() \
    do { \
        lua_settop(L, init_top); \
        if (return_output) { \
            lua_pushstring(L, ""); \
        } \
        if (return_exitcode) { \
            lua_pushinteger(L, 127); \
        } \
        goto cleanup; \
    } while (0)

    if (!return_output && !return_exitcode) {
        // Yes, we invoke shell to spawn a process without waiting for its termination. Doing
        // otherwise would hilariously increase the complexity (execvp() is not async-safe (see
        // http://www.linuxprogrammingblog.com/threads-and-fork-think-twice-before-using-them), so
        // forking twice means reimplementing its job of searching for the binary over $PATH), or
        // leak zombie processes.
        LS_VECTOR_PUSH(argv, ls_xstrdup(LUASTATUS_POSIX_SH_PATH));
        LS_VECTOR_PUSH(argv, ls_xstrdup("-c"));
        LS_VECTOR_PUSH(argv, ls_xstrdup("exec \"$@\"&"));
        LS_VECTOR_PUSH(argv, ls_xstrdup("")); // this will be $0
    }

    if (init_top != 1) {
        USAGE_ERR("%s: expected exactly one argument, found %d", funcname, init_top);
    }
    if (!lua_istable(L, 1)) {
        USAGE_ERR("%s: expected table, found %s", funcname, luaL_typename(L, 1));
    }
    size_t nrealargs = 0;
    LS_LUA_TRAVERSE(L, 1) {
        if (!lua_isnumber(L, LS_LUA_TRAVERSE_KEY)) {
            USAGE_ERR("%s: table key: expected number, found %s", funcname, luaL_typename(L, -2));
        }
        if (!lua_isstring(L, LS_LUA_TRAVERSE_VALUE)) {
            USAGE_ERR("%s: table element: expected string, found %s", funcname,
                      luaL_typename(L, LS_LUA_TRAVERSE_VALUE));
        }
        LS_VECTOR_PUSH(argv, ls_xstrdup(lua_tostring(L, LS_LUA_TRAVERSE_VALUE)));
        ++nrealargs;
    }
    if (!nrealargs) {
        USAGE_ERR("%s: empty table", funcname);
    }

    LS_VECTOR_PUSH(argv, NULL);

    if ((pid = ls_spawnp_pipe(argv.data[0], return_output ? &pipe_fd : NULL, argv.data)) < 0) {
        pipe_fd = -1;
        LS_WITH_ERRSTR(s, errno,
            internal_logf(LUASTATUS_ERR, "%s: can't spawn child process: %s", funcname, s);
        );
        SPAWN_ERR();
    }

    if (return_output) {
        LS_VECTOR_RESERVE(output_buf, 1024);
        if (ls_full_read_append(pipe_fd, &output_buf.data, &output_buf.size,
                                &output_buf.capacity) < 0)
        {
            LS_WITH_ERRSTR(s, errno,
                internal_logf(LUASTATUS_ERR, "%s: read: %s", funcname, s);
            );
            SPAWN_ERR();
        }
        lua_pushlstring(L, output_buf.data, dollar_len(output_buf.data, output_buf.size));
    }

    if (return_exitcode) {
        int status;
        int r = waitpid(pid, &status, 0);
        pid = -1;
        if (r < 0) {
            LS_WITH_ERRSTR(s, errno,
                internal_logf(LUASTATUS_ERR, "%s: waitpid: %s", funcname, s);
            );
            SPAWN_ERR();
        }
        lua_pushinteger(L, get_rc(status));
    }

cleanup:
    for (size_t i = 0; i < argv.size; ++i) {
        free(argv.data[i]);
    }
    LS_VECTOR_FREE(argv);
    LS_VECTOR_FREE(output_buf);
    ls_close(pipe_fd);
    if (pid > 0) {
        waitpid(pid, NULL, 0);
    }
    if (luaerr) {
        return luaL_error(L, "%s", luaerr_msg);
    } else {
        return lua_gettop(L) - init_top;
    }
}

static
int
l_dollar(lua_State *L)
{
    return lspawn_func(L, "luastatus.dollar", true, true);
}

static
int
l_rc(lua_State *L)
{
    return lspawn_func(L, "luastatus.rc", false, true);
}

static
int
l_spawn(lua_State *L)
{
    return lspawn_func(L, "luastatus.spawn", false, false);
}

void
lualibs_inject(lua_State *L)
{
    ls_lua_pushglobaltable(L); // L: _G
    ls_lua_rawgetf(L, "os"); // L: _G os

    lua_pushcfunction(L, l_os_execute); // L: _G os l_os_execute
    ls_lua_rawsetf(L, "execute"); // L: _G os

    lua_pushcfunction(L, l_os_exit); // L: _G os l_os_exit
    ls_lua_rawsetf(L, "exit"); // L: _G os

    lua_pushcfunction(L, l_os_getenv); // L: _G os l_os_getenv
    ls_lua_rawsetf(L, "getenv"); // L: _G os

    lua_pushcfunction(L, l_os_setlocale); // L: _G os l_os_setlocale
    ls_lua_rawsetf(L, "setlocale"); // L: _G os

    lua_pop(L, 1); // L: _G
    lua_newtable(L); // L: _G table

    lua_pushcfunction(L, l_dollar); // L: _G table l_dollar
    ls_lua_rawsetf(L, "dollar"); // L: _G table

    lua_pushcfunction(L, l_rc); // L: _G table l_rc
    ls_lua_rawsetf(L, "rc"); // L: _G table

    lua_pushcfunction(L, l_spawn); // L: table l_spawn
    ls_lua_rawsetf(L, "spawn"); // L: _G table

    ls_lua_rawsetf(L, "luastatus"); // L: _G
    lua_pop(L, 1); // L: -
}

void
lualibs_register_funcs(Widget *w, Barlib *barlib)
{
    assert(w->state == WIDGET_STATE_INITED);
    assert(barlib->state == BARLIB_STATE_INITED);

    lua_State *L = w->L; // L: -
    ls_lua_pushglobaltable(L); // L: _G
    ls_lua_rawgetf(L, "luastatus"); // L: _G luastatus
    if (!lua_istable(L, -1)) {
        internal_logf(LUASTATUS_WARN,
                      "widget '%s': 'luastatus' is not a table anymore, will not register "
                      "plugin/barlib functions", w->filename);
        goto done;
    }
    if (w->plugin.iface.register_funcs) {
        lua_newtable(L); // L: _G luastatus table
        w->plugin.iface.register_funcs(&w->data, L);
        assert(lua_gettop(L) == 3); // L: _G luastatus table
        ls_lua_rawsetf(L, "plugin"); // L: _G luastatus
    }
    if (barlib->iface.register_funcs) {
        lua_newtable(L); // L: _G luastatus table
        barlib->iface.register_funcs(&barlib->data, L);
        assert(lua_gettop(L) == 3); // L: _G luastatus table
        ls_lua_rawsetf(L, "barlib"); // L: _G luastatus
    }
done:
    lua_pop(L, 2); // L: -
}
