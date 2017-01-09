#include "include/plugin.h"
#include "include/plugin_logf_macros.h"
#include "include/plugin_utils.h"

#include <lua.h>
#include <errno.h>
#include <stdlib.h>

#include <time.h>
#include <sys/socket.h>
#include <netdb.h>

#include <sys/select.h>
#include <signal.h>

#include <stdio.h>
#include <string.h>

#include "libls/alloc_utils.h"
#include "libls/errno_utils.h"
#include "libls/time_utils.h"
#include "libls/sprintf_utils.h"
#include "libls/osdep.h"
#include "libls/wakeup_fifo.h"

#include "plugin_run_data.h"
#include "call_state.h"

typedef struct {
    char *hostname;
    int port;
    char *password;
    struct timespec timeout;
    struct timespec retry_in;
    char *retry_fifo;
} Priv;

void
priv_destroy(Priv *p)
{
    free(p->hostname);
    free(p->password);
    free(p->retry_fifo);
}

LuastatusPluginInitResult
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .hostname = NULL,
        .port = 6600,
        .password = NULL,
        .timeout = ls_timespec_invalid,
        .retry_in = (struct timespec) {10, 0},
        .retry_fifo = NULL,
    };

    PU_MAYBE_VISIT_STR("hostname", s,
        p->hostname = ls_xstrdup(s);
    );

    PU_MAYBE_VISIT_NUM("port", n,
        if (n < 0 || n > 65535) {
            LUASTATUS_FATALF(pd, "port (%g) is not a valid port number", (double) n);
            goto error;
        }
        p->port = n;
    );

    PU_MAYBE_VISIT_STR("password", s,
        p->password = ls_xstrdup(s);
    );

    PU_MAYBE_VISIT_NUM("timeout", n,
        if (ls_timespec_is_invalid(p->timeout = ls_timespec_from_seconds(n)) && n >= 0) {
            LUASTATUS_FATALF(pd, "'timeout' is invalid");
            goto error;
        }
    );

    PU_MAYBE_VISIT_NUM("retry_in", n,
        if (ls_timespec_is_invalid(p->retry_in = ls_timespec_from_seconds(n)) && n >= 0) {
            LUASTATUS_FATALF(pd, "'retry_in' is invalid");
            goto error;
        }
    );

    PU_MAYBE_VISIT_STR("retry_fifo", s,
        p->retry_fifo = ls_xstrdup(s);
    );

    return LUASTATUS_PLUGIN_INIT_RESULT_OK;

error:
    priv_destroy(p);
    free(p);
    return LUASTATUS_PLUGIN_INIT_RESULT_ERR;
}

int
tcp_open(LuastatusPluginData *pd, const char *hostname, const char *service)
{
    struct addrinfo* ai = NULL;
    int fd = -1;

    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = 0,
        .ai_flags = AI_ADDRCONFIG,
    };

    int gai_r = getaddrinfo(hostname, service, &hints, &ai);
    if (gai_r) {
        if (gai_r == EAI_SYSTEM) {
            LS_WITH_ERRSTR(s, errno,
                LUASTATUS_ERRF(pd, "getaddrinfo: %s", s);
            );
        } else {
            LUASTATUS_ERRF(pd, "getaddrinfo: %s", gai_strerror(gai_r));
        }
        ai = NULL;
        goto cleanup;
    }

    for (struct addrinfo *pai = ai; pai; pai = pai->ai_next) {
        if ((fd = ls_cloexec_socket(pai->ai_family, pai->ai_socktype, pai->ai_protocol)) < 0) {
            LS_WITH_ERRSTR(s, errno,
                LUASTATUS_WARNF(pd, "(candiate) socket: %s", s);
            );
            continue;
        }
        if (connect(fd, pai->ai_addr, pai->ai_addrlen) < 0) {
            LS_WITH_ERRSTR(s, errno,
                LUASTATUS_WARNF(pd, "(candiate) connect: %s", s);
            );
            ls_close(fd);
            fd = -1;
            continue;
        }
        break;
    }
    if (fd < 0) {
        LUASTATUS_ERRF(pd, "can't connect to any of the candidates");
    }

cleanup:
    if (ai) {
        freeaddrinfo(ai);
    }
    return fd;
}

// string length without trailing newlines
static inline
size_t
dollar_strlen(const char *s)
{
    size_t len = strlen(s);
    while (len && s[len - 1] == '\n') {
        --len;
    }
    return len;
}

// If line is of form "<key>: <value>\n", performs the assignment
//     t[key] = value
// where t is the table on top of the L's stack.
void
line_setfield(lua_State *L, const char *line)
{
    const char *colon_pos = strchr(line, ':');
    if (!colon_pos || colon_pos[1] != ' ') {
        return;
    }
    const char *value_pos = colon_pos + 2;
    // L: table
    lua_pushlstring(L, line, colon_pos - line); // L: table key
    lua_pushlstring(L, value_pos, dollar_strlen(value_pos)); // L: table key value
    lua_settable(L, -3); // L: table
}

typedef enum {
    RESP_TYPE_OK,
    RESP_TYPE_ACK,
    RESP_TYPE_OTHER,
} ResponseType;

static inline
ResponseType
response_type(const char *buf)
{
    if (strcmp(buf, "OK\n") == 0) {
        return RESP_TYPE_OK;
    }
    if (strncmp(buf, "ACK ", 4) == 0) {
        return RESP_TYPE_ACK;
    }
    return RESP_TYPE_OTHER;
}

// Code above is pretty ugly and spaghetti-like. Rewrite it if you can.
void
interact(int fd, PluginRunData data)
{
    Priv *p = data.pd->priv;

    FILE *f = NULL;
    int fd_to_close = fd;
    char *buf = NULL;
    size_t nbuf = 0;
    CallState s = CALL_STATE_INITIALIZER;

    if (!(f = fdopen(fd, "r+"))) {
        LS_WITH_ERRSTR(str, errno,
            LUASTATUS_ERRF(data.pd, "fdopen: %s", str);
        );
        goto error;
    }
    fd_to_close = -1;

    // read and check the greeting
    if (getline(&buf, &nbuf, f) < 0) {
        goto io_error;
    }
    if (strncmp(buf, "OK MPD ", 7) != 0) {
        LUASTATUS_ERRF(data.pd, "bad greeting: %.*s", (int) dollar_strlen(buf), buf);
        goto error;
    }

    if (p->password) {
        if (fprintf(f, "password %s\n", p->password) < 0 || fflush(f) < 0) {
            goto io_error;
        }
        if (getline(&buf, &nbuf, f) < 0) {
            goto io_error;
        }
        if (response_type(buf) != RESP_TYPE_OK) {
            LUASTATUS_ERRF(data.pd, "(password) server said: %.*s", (int) dollar_strlen(buf), buf);
            goto error;
        }
    }

#define UNTIL_OK(...) \
    do { \
        if (getline(&buf, &nbuf, f) < 0) { \
            goto io_error; \
        } \
        ResponseType r_ = response_type(buf); \
        if (r_ == RESP_TYPE_OK) { \
            break; \
        } else if (r_ == RESP_TYPE_ACK) { \
            LUASTATUS_ERRF(data.pd, "server said: %.*s", (int) dollar_strlen(buf), buf); \
            goto error; \
        } else { \
            __VA_ARGS__ \
        } \
    } while (1) /*(!)*/

    sigset_t allsigs;
    if (sigfillset(&allsigs) < 0) {
        LS_WITH_ERRSTR(str, errno,
            LUASTATUS_ERRF(data.pd, "sigfillset: %s", str);
        );
        goto error;
    }

    if (fd >= FD_SETSIZE && !ls_timespec_is_invalid(p->timeout)) {
        LUASTATUS_WARNF(data.pd, "connection file descriptor is too large, will not report time "
                                 "outs");
    }

    fd_set fds;
    FD_ZERO(&fds);

    while (1) {
        // query and make a call
        call_state_call_begin(&s, data);

        // set "what"
        lua_newtable(s.L); // s.L: table
        lua_pushstring(s.L, "update"); // s.L: table "update"
        lua_setfield(s.L, -2, "what"); // s.L: table

        // set "currentsong"
        if (fputs("currentsong\n", f) < 0 || fflush(f) < 0) {
            goto io_error;
        }
        lua_newtable(s.L); // s.L: table table
        UNTIL_OK(
            line_setfield(s.L, buf);
        );
        lua_setfield(s.L, -2, "song"); // s.L: table

        // set "status"
        if (fputs("status\n", f) < 0 || fflush(f) < 0) {
            goto io_error;
        }
        lua_newtable(s.L); // s.L: table table
        UNTIL_OK(
            line_setfield(s.L, buf);
        );
        lua_setfield(s.L, -2, "status"); // s.L: table

        // done
        call_state_call_end(&s, data);

        // wait
        if (fputs("idle player mixer\n", f) < 0 || fflush(f) < 0) {
            goto io_error;
        }

        if (fd < FD_SETSIZE && !ls_timespec_is_invalid(p->timeout)) {
            while (1) {
                FD_SET(fd, &fds);
                int r = pselect(fd + 1, &fds, NULL, NULL, &p->timeout, &allsigs);
                if (r < 0) {
                    LS_WITH_ERRSTR(str, errno,
                        LUASTATUS_ERRF(data.pd, "pselect (on connection file descriptor): %s", str);
                    );
                    goto error;
                } else if (r == 0) {
                    call_state_call_begin(&s, data);
                    lua_newtable(s.L); // s.L: table
                    lua_pushstring(s.L, "timeout"); // s.L: table "timeout"
                    lua_setfield(s.L, -2, "what"); // s.L: table
                    call_state_call_end(&s, data);
                } else {
                    break;
                }
            }
        }

        UNTIL_OK(
            // do nothing
        );
    }
#undef UNTIL_OK

io_error:
    if (feof(f)) {
        LUASTATUS_ERRF(data.pd, "server closed the connection");
    } else {
        LS_WITH_ERRSTR(str, errno,
            LUASTATUS_ERRF(data.pd, "I/O error: %s", str);
        );
    }

error:
    call_state_call_begin_or_reuse(&s, data);
    lua_newtable(s.L); // s.L: table
    lua_pushstring(s.L, "error"); // s.L: table "error"
    lua_setfield(s.L, -2, "what"); // s.L: table
    call_state_call_end(&s, data);

    if (f) {
        fclose(f);
    }
    ls_close(fd_to_close);
    free(buf);
}

void
run(LuastatusPluginData *pd, LuastatusPluginCallBegin call_begin, LuastatusPluginCallEnd call_end)
{
    Priv *p = pd->priv;

    LSWakeupFifo w;
    ls_wakeup_fifo_init(&w);

    sigset_t allsigs;
    if (sigfillset(&allsigs) < 0) {
        LS_WITH_ERRSTR(s, errno,
            LUASTATUS_FATALF(pd, "sigfillset: %s", s);
        );
        goto error;
    }
    w.fifo = p->retry_fifo;
    w.timeout = &p->retry_in;
    w.sigmask = &allsigs;

    char portstr[8];
    ls_xsnprintf(portstr, sizeof(portstr), "%d", p->port);

    while (1) {
        lua_State *L = call_begin(pd->userdata);
        lua_newtable(L); // L: table
        lua_pushstring(L, "connecting"); // L: table "connecting"
        lua_setfield(L, -2, "what"); // L: table
        call_end(pd->userdata);

        int fd = tcp_open(pd, p->hostname, portstr);
        if (fd >= 0) {
            interact(fd, (PluginRunData) {.pd = pd, .call_begin = call_begin,
                                          .call_end = call_end});
        }

        if (ls_timespec_is_invalid(p->retry_in)) {
            LUASTATUS_FATALF(pd, "an error occurred; not retrying as requested");
            goto error;
        }

        if (ls_wakeup_fifo_open(&w) < 0) {
            LS_WITH_ERRSTR(s, errno,
                LUASTATUS_WARNF(pd, "open: %s: %s", p->retry_fifo, s);
            );
        }
        if (ls_wakeup_fifo_pselect(&w) < 0) {
            LS_WITH_ERRSTR(s, errno,
                LUASTATUS_FATALF(pd, "pselect (on retry_fifo): %s", s);
            );
            goto error;
        }
    }

error:
    ls_wakeup_fifo_destroy(&w);
}

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
