#include "include/plugin.h"
#include "include/plugin_logf_macros.h"
#include "include/plugin_utils.h"

#include <lua.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "libls/vector.h"
#include "libls/alloc_utils.h"
#include "libls/errno_utils.h"
#include "libls/io_utils.h"
#include "libls/osdep.h"
#include "libls/lua_utils.h"

extern char **environ;

typedef struct {
    LS_VECTOR_OF(char*) args;
    char delim;
} Priv;

void
priv_destroy(Priv *p)
{
    for (size_t i = 0; i < p->args.size; ++i) {
        free(p->args.data[i]);
    }
    LS_VECTOR_FREE(p->args);
}

LuastatusPluginInitResult
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .args = LS_VECTOR_NEW_RESERVE(char*, 8),
        .delim = '\n',
    };

    PU_MAYBE_TRAVERSE_TABLE("args",
        PU_CHECK_TYPE_AT(LS_LUA_TRAVERSE_KEY, "'args' key", LUA_TNUMBER);
        PU_VISIT_STR_AT(LS_LUA_TRAVERSE_VALUE, "'args' element", s,
            LS_VECTOR_PUSH(p->args, ls_xstrdup(s));
        );
    );
    if (!p->args.size) {
        LUASTATUS_FATALF(pd, "args not specified or empty");
        goto error;
    }
    LS_VECTOR_PUSH(p->args, NULL);

    PU_MAYBE_VISIT_LSTR("delimiter", s, n,
        if (n != 1) {
            LUASTATUS_FATALF(pd, n ? "delimiter is longer than one symbol" : "delimiter is empty");
            goto error;
        }
        p->delim = s[0];
    );

    return LUASTATUS_PLUGIN_INIT_RESULT_OK;

error:
    priv_destroy(p);
    free(p);
    return LUASTATUS_PLUGIN_INIT_RESULT_ERR;
}

void
run(
    LuastatusPluginData *pd,
    LuastatusPluginCallBegin call_begin,
    LuastatusPluginCallEnd call_end)
{
    Priv *p = pd->priv;

    pid_t pid = -1;
    int fd = -1;
    FILE *f = NULL;
    size_t nbuf = 1024;
    char *buf = NULL;

    if ((pid = ls_spawnp_pipe(p->args.data[0], &fd, p->args.data)) < 0) {
        LS_WITH_ERRSTR(s, errno,
            LUASTATUS_FATALF(pd, "spawn failed: %s", s);
        );
        fd = -1;
        goto error;
    }

    if (!(f = fdopen(fd, "r"))) {
        LS_WITH_ERRSTR(s, errno,
            LUASTATUS_FATALF(pd, "fdopen: %s", s);
        );
        goto error;
    }
    fd = -1;

    int delim = (unsigned char) p->delim;
    while (1) {
        ssize_t r = getdelim(&buf, &nbuf, delim, f);
        if (r < 0) {
            if (feof(f)) {
                LUASTATUS_FATALF(pd, "child process closed its stdout");
            } else {
                LS_WITH_ERRSTR(s, errno,
                    LUASTATUS_FATALF(pd, "read error: %s", s);
                );
            }
            goto error;
        } else if (r > 0) {
            lua_State *L = call_begin(pd->userdata);
            lua_pushlstring(L, buf, r - 1);
            call_end(pd->userdata);
        }
    }

error:
    waitpid(pid, NULL, 0);
    if (f) {
        fclose(f);
    }
    ls_close(fd);
    free(buf);
}

static
void
destroy(LuastatusPluginData *pd)
{
    priv_destroy(pd->priv);
    free(pd->priv);
}

LuastatusPluginIface luastatus_plugin_iface = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
