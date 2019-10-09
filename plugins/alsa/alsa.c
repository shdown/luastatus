#include <errno.h>
#include <lua.h>
#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <poll.h>
#include <alsa/asoundlib.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"
#include "include/plugin_utils.h"

#include "libls/alloc_utils.h"
#include "libls/cstring_utils.h"
#include "libls/evloop_utils.h"

typedef struct {
    char *card;
    char *channel;
    bool capture;
    bool in_db;
    int timeout_ms;
    LSSelfPipe self_pipe;
} Priv;

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->card);
    free(p->channel);
    ls_self_pipe_close(&p->self_pipe);
    free(p);
}

static
int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .card = NULL,
        .channel = NULL,
        .capture = false,
        .in_db = false,
        .timeout_ms = -1,
        .self_pipe = LS_SELF_PIPE_NEW(),
    };

    PU_MAYBE_VISIT_STR_FIELD(-1, "card", "'card'", s,
        p->card = ls_xstrdup(s);
    );
    if (!p->card) {
        p->card = ls_xstrdup("default");
    }

    PU_MAYBE_VISIT_STR_FIELD(-1, "channel", "'channel'", s,
        p->channel = ls_xstrdup(s);
    );
    if (!p->channel) {
        p->channel = ls_xstrdup("Master");
    }

    PU_MAYBE_VISIT_BOOL_FIELD(-1, "capture", "'capture'", b,
        p->capture = b;
    );

    PU_MAYBE_VISIT_BOOL_FIELD(-1, "in_db", "'in_db'", b,
        p->in_db = b;
    );

    PU_MAYBE_VISIT_NUM_FIELD(-1, "timeout", "'timeout'", nsec,
        // Note: this also implicitly checks that /nsec/ is not NaN.
        if (nsec >= 0) {
            double nmillis = nsec * 1000;
            if (nmillis > INT_MAX) {
                LS_FATALF(pd, "'timeout' is too large");
                goto error;
            }
            p->timeout_ms = nmillis;
        }
    );

    PU_MAYBE_VISIT_BOOL_FIELD(-1, "make_self_pipe", "'make_self_pipe'", b,
        if (b) {
            if (ls_self_pipe_open(&p->self_pipe) < 0) {
                LS_FATALF(pd, "ls_self_pipe_open: %s", ls_strerror_onstack(errno));
                goto error;
            }
        }
    );

    return LUASTATUS_OK;

error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static
void
register_funcs(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv;
    // L: table
    ls_self_pipe_push_luafunc(&p->self_pipe, L); // L: table func
    lua_setfield(L, -2, "wake_up"); // L: table
}

static
bool
card_has_nicename(const char *realname, snd_ctl_card_info_t *info, const char *nicename)
{
    snd_ctl_t *ctl;
    if (snd_ctl_open(&ctl, realname, 0) < 0) {
        return false;
    }
    bool r = snd_ctl_card_info(ctl, info) >= 0 &&
             strcmp(snd_ctl_card_info_get_name(info), nicename) == 0;
    snd_ctl_close(ctl);
    return r;
}

static
char *
xalloc_card_realname(const char *nicename)
{
    snd_ctl_card_info_t *info;
    if (snd_ctl_card_info_malloc(&info) < 0) {
        ls_oom();
    }
    enum { NBUF = 32 };
    char *buf = LS_XNEW(char, NBUF);

    int rcard = -1;
    while (snd_card_next(&rcard) >= 0 && rcard >= 0) {
        snprintf(buf, NBUF, "hw:%d", rcard);
        if (card_has_nicename(buf, info, nicename)) {
            goto cleanup;
        }
    }

    free(buf);
    buf = NULL;

cleanup:
    snd_ctl_card_info_free(info);
    return buf;
}

typedef struct {
    int (*get_range)(snd_mixer_elem_t *, long *, long *);
    int (*get_cur)(snd_mixer_elem_t *, snd_mixer_selem_channel_id_t, long *);
    int (*get_switch)(snd_mixer_elem_t *, snd_mixer_selem_channel_id_t, int *);
} GetVolFuncs;

static
GetVolFuncs
select_gv_funcs(bool capture, bool in_db)
{
    return (GetVolFuncs) {
        .get_range =
            capture ? (in_db ? snd_mixer_selem_get_capture_dB_range
                             : snd_mixer_selem_get_capture_volume_range)
                    : (in_db ? snd_mixer_selem_get_playback_dB_range
                             : snd_mixer_selem_get_playback_volume_range),
        .get_cur =
            capture ? (in_db ? snd_mixer_selem_get_capture_dB
                             : snd_mixer_selem_get_capture_volume)
                    : (in_db ? snd_mixer_selem_get_playback_dB
                             : snd_mixer_selem_get_playback_volume),
        .get_switch =
            capture ? snd_mixer_selem_get_capture_switch
                    : snd_mixer_selem_get_playback_switch,
    };
}

static
void
push_vol_info(lua_State *L, snd_mixer_elem_t *elem, GetVolFuncs gv_funcs)
{
    lua_createtable(L, 0, 2); // L: table

    lua_createtable(L, 0, 3); // L: table table
    long pmin, pmax;
    if (gv_funcs.get_range(elem, &pmin, &pmax) >= 0) {
        lua_pushnumber(L, pmin); // L: table table pmin
        lua_setfield(L, -2, "min"); // L: table table
        lua_pushnumber(L, pmax); // L: table table pmax
        lua_setfield(L, -2, "max"); // L: table table
    }
    long pcur;
    if (gv_funcs.get_cur(elem, 0, &pcur) >= 0) {
        lua_pushnumber(L, pcur); // L: table table pcur
        lua_setfield(L, -2, "cur"); // L: table table
    }
    lua_setfield(L, -2, "vol"); // L: table

    int notmute;
    if (gv_funcs.get_switch(elem, 0, &notmute) >= 0) {
        lua_pushboolean(L, !notmute); // L: table !notmute
        lua_setfield(L, -2, "mute"); // L: table
    }
    // L: table
}

typedef struct {
    struct pollfd *data;
    size_t size;
    size_t nprefix;
} PollFdSet;

static inline
PollFdSet
pollfd_set_new(const struct pollfd *prefix, size_t nprefix)
{
    return (PollFdSet) {
        .data = ls_xmemdup(prefix, nprefix * sizeof(struct pollfd)),
        .size = nprefix,
        .nprefix = nprefix,
    };
}

static inline
void
pollfd_set_resize(PollFdSet *s, size_t n)
{
    if (s->size != n) {
        s->data = ls_xrealloc(s->data, n, sizeof(struct pollfd));
        s->size = n;
    }
}

static inline
void
pollfd_set_free(PollFdSet s)
{
    free(s.data);
}

static
bool
iteration(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;
    bool ret = false;

    snd_mixer_t *mixer = NULL;
    snd_mixer_selem_id_t *sid = NULL;
    char *realname = NULL;
    PollFdSet pollfds = ls_self_pipe_is_opened(&p->self_pipe)
        ? pollfd_set_new((struct pollfd [1]) {{.fd = p->self_pipe.fds[0], .events = POLLIN}}, 1)
        : pollfd_set_new(NULL, 0);

    if (!(realname = xalloc_card_realname(p->card))) {
        realname = ls_xstrdup(p->card);
    }

#define ALSA_CALL(Func_, ...) \
    do { \
        int r_; \
        while ((r_ = Func_(__VA_ARGS__)) == -EINTR) {} \
        if (r_ < 0) { \
            LS_FATALF(pd, "%s: %s", #Func_, snd_strerror(r_)); \
            goto error; \
        } \
    } while (0)

    ALSA_CALL(snd_mixer_open, &mixer, 0);
    ALSA_CALL(snd_mixer_attach, mixer, realname);
    ALSA_CALL(snd_mixer_selem_register, mixer, NULL, NULL);
    ALSA_CALL(snd_mixer_load, mixer);
    ALSA_CALL(snd_mixer_selem_id_malloc, &sid);

    snd_mixer_selem_id_set_name(sid, p->channel);
    snd_mixer_elem_t *elem = snd_mixer_find_selem(mixer, sid);
    if (!elem) {
        LS_FATALF(pd, "can't find channel '%s'", p->channel);
        goto error;
    }

    ret = true;

    GetVolFuncs gv_funcs = select_gv_funcs(p->capture, p->in_db);
    bool is_timeout = false;
    while (1) {
        lua_State *L = funcs.call_begin(pd->userdata);
        if (is_timeout) {
            lua_pushnil(L); // L: nil
        } else {
            push_vol_info(L, elem, gv_funcs); // L: table
        }
        funcs.call_end(pd->userdata);

        pollfd_set_resize(&pollfds, pollfds.nprefix + snd_mixer_poll_descriptors_count(mixer));
        ALSA_CALL(snd_mixer_poll_descriptors,
            mixer,
            pollfds.data + pollfds.nprefix,
            pollfds.size - pollfds.nprefix);

        // TODO: call sigprocmask() before and after poll() to emulate ppoll() (if it worth the
        // trouble).
        const int r = poll(pollfds.data, pollfds.size, p->timeout_ms);
        if (r < 0) {
            if (errno == EINTR) {
                if (p->timeout_ms == -1) {
                    is_timeout = false;
                    continue;
                } else {
                    LS_WARNF(pd, "poll: interrupted by signal, reporting as timeout");
                    is_timeout = true;
                    continue;
                }
            } else {
                LS_FATALF(pd, "poll: %s", ls_strerror_onstack(errno));
                goto error;
            }

        } else if (r == 0) {
            is_timeout = true;

        } else {
            is_timeout = false;

            if (pollfds.nprefix && (pollfds.data[0].revents & POLLIN)) {
                ssize_t unused = read(p->self_pipe.fds[0], (char [1]) {'\0'}, 1);
                (void) unused;
            }
        }

        unsigned short revents;
        ALSA_CALL(snd_mixer_poll_descriptors_revents, mixer,
                  pollfds.data + pollfds.nprefix, pollfds.size - pollfds.nprefix, &revents);
        if (revents & (POLLERR | POLLNVAL)) {
            LS_ERRF(pd, "snd_mixer_poll_descriptors_revents() reported an error condition");
            goto error;
        }
        if (revents & POLLIN) {
            ALSA_CALL(snd_mixer_handle_events, mixer);
        }
    }
#undef ALSA_CALL
error:
    if (sid) {
        snd_mixer_selem_id_free(sid);
    }
    if (mixer) {
        snd_mixer_close(mixer);
    }
    free(realname);
    pollfd_set_free(pollfds);
    return ret;
}

static
void
run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    while (1) {
        if (!iteration(pd, funcs)) {
            nanosleep((struct timespec[1]) {{.tv_sec = 5}}, NULL);
        }
    }
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .run = run,
    .destroy = destroy,
};
