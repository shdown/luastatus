#include "include/plugin.h"
#include "include/sayf_macros.h"
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

typedef struct {
    LS_VECTOR_OF(char *) args;
    char delim;
} Priv;

void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    for (size_t i = 0; i < p->args.size; ++i) {
        free(p->args.data[i]);
    }
    LS_VECTOR_FREE(p->args);
    free(p);
}

int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .args = LS_VECTOR_NEW_RESERVE(char *, 8),
        .delim = '\n',
    };

    PU_TRAVERSE_TABLE("args",
        PU_CHECK_TYPE_AT(LS_LUA_KEY, "'args' key", LUA_TNUMBER);
        PU_VISIT_STR_AT(LS_LUA_VALUE, "'args' element", s,
            LS_VECTOR_PUSH(p->args, ls_xstrdup(s));
        );
    );
    if (!p->args.size) {
        LS_FATALF(pd, "args are empty");
        goto error;
    }
    LS_VECTOR_PUSH(p->args, NULL);

    PU_MAYBE_VISIT_LSTR("delimiter", s, n,
        if (n != 1) {
            LS_FATALF(pd, n ? "delimiter is longer than one symbol" : "delimiter is empty");
            goto error;
        }
        p->delim = s[0];
    );

    return LUASTATUS_RES_OK;

error:
    destroy(pd);
    return LUASTATUS_RES_ERR;
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
            LS_FATALF(pd, "spawn failed: %s", s);
        );
        fd = -1;
        goto error;
    }

    if (!(f = fdopen(fd, "r"))) {
        LS_WITH_ERRSTR(s, errno,
            LS_FATALF(pd, "fdopen: %s", s);
        );
        goto error;
    }
    fd = -1;

    const char delim = p->delim;
    const int idelim = (unsigned char) delim;
    while (1) {
        const ssize_t r = getdelim(&buf, &nbuf, idelim, f);
        if (r < 0) {
            if (feof(f)) {
                LS_FATALF(pd, "child process closed its stdout");
            } else {
                LS_WITH_ERRSTR(s, errno,
                    LS_FATALF(pd, "read error: %s", s);
                );
            }
            goto error;
        } else if (r > 0 && buf[r - 1] == delim) {
            lua_State *L = call_begin(pd->userdata);
            lua_pushlstring(L, buf, r - 1);
            call_end(pd->userdata);
        }
    }

error:
    if (pid > 0) {
        waitpid(pid, NULL, 0);
    }
    if (f) {
        fclose(f);
    }
    ls_close(fd);
    free(buf);
}

LuastatusPluginIface luastatus_plugin_iface = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
