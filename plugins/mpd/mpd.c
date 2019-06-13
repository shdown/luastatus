#include <lua.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/select.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"
#include "include/plugin_utils.h"

#include "libls/lua_utils.h"
#include "libls/string_.h"
#include "libls/alloc_utils.h"
#include "libls/vector.h"
#include "libls/cstring_utils.h"
#include "libls/time_utils.h"
#include "libls/evloop_utils.h"
#include "libls/strarr.h"
#include "libls/sig_utils.h"

#include "connect.h"
#include "proto.h"

typedef struct {
    char *hostname;
    int port;
    char *password;
    struct timespec timeout;
    struct timespec retry_in;
    char *retry_fifo;
    char *idle_str;
} Priv;

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->hostname);
    free(p->password);
    free(p->retry_fifo);
    free(p->idle_str);
    free(p);
}

static
int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .hostname = NULL,
        .port = 6600,
        .password = NULL,
        .timeout = ls_timespec_invalid,
        .retry_in = {.tv_sec = 10},
        .retry_fifo = NULL,
        .idle_str = NULL,
    };
    LSString idle_str = ls_string_new_from_s("idle");

    PU_MAYBE_VISIT_STR_FIELD(-1, "hostname", "'hostname'", s,
        p->hostname = ls_xstrdup(s);
    );

    PU_MAYBE_VISIT_NUM_FIELD(-1, "port", "'port'", n,
        if (n < 0 || n > 65535) {
            LS_FATALF(pd, "port (%g) is not a valid port number", (double) n);
            goto error;
        }
        p->port = n;
    );

    PU_MAYBE_VISIT_STR_FIELD(-1, "password", "'password'", s,
        if ((strchr(s, '\n'))) {
            LS_FATALF(pd, "password contains a line break");
            goto error;
        }
        p->password = ls_xstrdup(s);
    );

    PU_MAYBE_VISIT_NUM_FIELD(-1, "timeout", "'timeout'", n,
        if (ls_timespec_is_invalid(p->timeout = ls_timespec_from_seconds(n)) && n >= 0) {
            LS_FATALF(pd, "'timeout' is invalid");
            goto error;
        }
    );

    PU_MAYBE_VISIT_NUM_FIELD(-1, "retry_in", "'retry_in'", n,
        if (ls_timespec_is_invalid(p->retry_in = ls_timespec_from_seconds(n)) && n >= 0) {
            LS_FATALF(pd, "'retry_in' is invalid");
            goto error;
        }
    );

    PU_MAYBE_VISIT_STR_FIELD(-1, "retry_fifo", "'retry_fifo'", s,
        p->retry_fifo = ls_xstrdup(s);
    );

    bool has_events = false;
    PU_MAYBE_VISIT_TABLE_FIELD(-1, "events", "'events'",
        has_events = true;
        PU_CHECK_TYPE(LS_LUA_KEY, "'events' key", LUA_TNUMBER);
        PU_VISIT_STR(LS_LUA_VALUE, "'events' element", s,
            ls_string_append_c(&idle_str, ' ');
            ls_string_append_s(&idle_str, s);
        );
    );
    if (!has_events) {
        ls_string_append_s(&idle_str, " mixer player");
    }
    ls_string_append_b(&idle_str, "\n", 2); // append '\n' and '\0'
    p->idle_str = idle_str.data;

    return LUASTATUS_OK;

error:
    LS_VECTOR_FREE(idle_str);
    destroy(pd);
    return LUASTATUS_ERR;
}

// Returns the length of /s/ without trailing newlines.
static inline
size_t
rstrip_nl_strlen(const char *s)
{
    size_t len = strlen(s);
    while (len && s[len - 1] == '\n') {
        --len;
    }
    return len;
}

// If /line/ is of form "key: value\n", appends /key/ and /value/ to /sa/.
static
void
kv_strarr_line_append(LSStringArray *sa, const char *line)
{
    const char *colon_pos = strchr(line, ':');
    if (!colon_pos || colon_pos[1] != ' ') {
        return;
    }
    const char *value_pos = colon_pos + 2;
    ls_strarr_append(sa, line, colon_pos - line);
    ls_strarr_append(sa, value_pos, rstrip_nl_strlen(value_pos));
}

static
void
kv_strarr_table_push(LSStringArray sa, lua_State *L)
{
    const size_t n = ls_strarr_size(sa);
    assert(n % 2 == 0);
    lua_createtable(L, 0, n / 2); // L: table
    for (size_t i = 0; i < n; i += 2) {
        size_t nkey;
        const char *key = ls_strarr_at(sa, i, &nkey);
        lua_pushlstring(L, key, nkey); // L: table key
        size_t nvalue;
        const char *value = ls_strarr_at(sa, i + 1, &nvalue);
        lua_pushlstring(L, value, nvalue); // L: table key value
        lua_settable(L, -3); // L: table
    }
}

static inline
void
report_status(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs, const char *what)
{
    lua_State *L = funcs.call_begin(pd->userdata);
    lua_createtable(L, 0, 1); // L: table
    lua_pushstring(L, what); // L: table what
    lua_setfield(L, -2, "what"); // L: table
    funcs.call_end(pd->userdata);
}

static
void
interact(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs, int fd)
{
    Priv *p = pd->priv;

    FILE *f = fdopen(fd, "r+");
    if (!f) {
        LS_ERRF(pd, "can't fdopen connection file descriptor %d: %s",
                fd, ls_strerror_onstack(errno));
        close(fd);
        return;
    }

    char *buf = NULL;
    size_t nbuf = 1024;
    LSStringArray kv_song   = ls_strarr_new();
    LSStringArray kv_status = ls_strarr_new();

#define GETLINE() \
    do { \
        if (getline(&buf, &nbuf, f) < 0) { \
            goto io_error; \
        } \
    } while (0)

#define WRITE(What_) \
    do { \
        fputs(What_, f); \
        fflush(f); \
        if (ferror(f)) { \
            goto io_error; \
        } \
    } while (0)

#define UNTIL_OK(...) \
    do { \
        GETLINE(); \
        const ResponseType rt_ = response_type(buf); \
        if (rt_ == RESP_OK) { \
            break; \
        } else if (rt_ == RESP_ACK) { \
            LS_ERRF(pd, "server said: %.*s", (int) rstrip_nl_strlen(buf), buf); \
            goto error; \
        } else { \
            __VA_ARGS__ \
        } \
    } while (/*note infinite cycle here*/ 1)

    // read and check the greeting
    GETLINE();
    if (strncmp(buf, "OK MPD ", 7) != 0) {
        LS_ERRF(pd, "bad greeting: %.*s", (int) rstrip_nl_strlen(buf), buf);
        goto error;
    }

    // send the password, if specified
    if (p->password) {
        fputs("password ", f);
        write_quoted(f, p->password);
        putc_unlocked('\n', f);
        fflush(f);
        if (ferror(f)) {
            goto io_error;
        }
        GETLINE();
        if (response_type(buf) != RESP_OK) {
            LS_ERRF(pd, "(password) server said: %.*s", (int) rstrip_nl_strlen(buf), buf);
            goto error;
        }
    }

    sigset_t allsigs;
    ls_xsigfillset(&allsigs);

    if (fd >= FD_SETSIZE && !ls_timespec_is_invalid(p->timeout)) {
        LS_WARNF(pd, "connection file descriptor is too large, will not report time outs");
    }

    fd_set fds;
    FD_ZERO(&fds);

    while (1) {
        WRITE("currentsong\n");
        UNTIL_OK(
            kv_strarr_line_append(&kv_song, buf);
        );

        WRITE("status\n");
        UNTIL_OK(
            kv_strarr_line_append(&kv_status, buf);
        );

        lua_State *L = funcs.call_begin(pd->userdata);
        lua_createtable(L, 0, 3); // L: table

        lua_pushstring(L, "update"); // L: table "update"
        lua_setfield(L, -2, "what"); // L: table

        kv_strarr_table_push(kv_song, L); // L: table table
        ls_strarr_clear(&kv_song);
        lua_setfield(L, -2, "song"); // L: table

        kv_strarr_table_push(kv_status, L); // L: table table
        ls_strarr_clear(&kv_status);
        lua_setfield(L, -2, "status"); // L: table

        funcs.call_end(pd->userdata);

        WRITE(p->idle_str);

        if (fd < FD_SETSIZE && !ls_timespec_is_invalid(p->timeout)) {
            while (1) {
                FD_SET(fd, &fds);
                int r = pselect(fd + 1, &fds, NULL, NULL, &p->timeout, &allsigs);
                if (r < 0) {
                    LS_ERRF(pd, "pselect (on connection file descriptor): %s",
                            ls_strerror_onstack(errno));
                    goto error;
                } else if (r == 0) {
                    report_status(pd, funcs, "timeout");
                } else {
                    break;
                }
            }
        }

        UNTIL_OK(
            // do nothing
        );
    }
#undef GETLINE
#undef WRITE
#undef UNTIL_OK

io_error:
    if (feof(f)) {
        LS_ERRF(pd, "connection closed");
    } else {
        LS_ERRF(pd, "I/O error: %s", ls_strerror_onstack(errno));
    }

error:
    fclose(f);
    free(buf);
    ls_strarr_destroy(kv_song);
    ls_strarr_destroy(kv_status);
}

static
void
run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    LSWakeupFifo w;
    ls_wakeup_fifo_init(&w, p->retry_fifo, NULL);

    char portstr[8];
    snprintf(portstr, sizeof(portstr), "%d", p->port);

    while (1) {
        report_status(pd, funcs, "connecting");

        int fd = (p->hostname && p->hostname[0] == '/')
            ? unixdom_open(pd, p->hostname)
            : inetdom_open(pd, p->hostname, portstr);
        if (fd >= 0) {
            interact(pd, funcs, fd);
        }

        if (ls_timespec_is_invalid(p->retry_in)) {
            LS_FATALF(pd, "an error occurred; not retrying as requested");
            goto error;
        }

        report_status(pd, funcs, "error");

        if (ls_wakeup_fifo_open(&w) < 0) {
            LS_WARNF(pd, "ls_wakeup_fifo_open: %s: %s", p->retry_fifo,
                errno == -EINVAL ? "The file is not a FIFO" : ls_strerror_onstack(errno));
        }
        if (ls_wakeup_fifo_wait(&w, p->retry_in) < 0) {
            LS_FATALF(pd, "ls_wakeup_fifo_wait: %s: %s", p->retry_fifo, ls_strerror_onstack(errno));
            goto error;
        }
    }

error:
    ls_wakeup_fifo_destroy(&w);
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
