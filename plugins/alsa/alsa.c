#include <errno.h>
#include <lua.h>
#include <stdbool.h>
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
#include "libls/io_utils.h"
#include "libls/cstring_utils.h"
#include "libls/osdep.h"

typedef struct {
    char *card;
    char *channel;
    bool capture;
    bool in_db;
    int self_pipe[2];
} Priv;

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->card);
    free(p->channel);
    close(p->self_pipe[0]);
    close(p->self_pipe[1]);
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
        .self_pipe = {-1, -1},
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

    PU_MAYBE_VISIT_BOOL_FIELD(-1, "make_self_pipe", "'make_self_pipe'", b,
        if (b) {
            if (ls_cloexec_pipe(p->self_pipe) < 0) {
                LS_FATALF(pd, "ls_cloexec_pipe: %s", ls_strerror_onstack(errno));
                p->self_pipe[0] = -1;
                p->self_pipe[1] = -1;
                goto error;
            }
            ls_make_nonblock(p->self_pipe[0]);
            ls_make_nonblock(p->self_pipe[1]);
        }
    );

    return LUASTATUS_OK;

error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static
int
l_wake_up(lua_State *L)
{
    LuastatusPluginData *pd = lua_touserdata(L, lua_upvalueindex(1));
    Priv *p = pd->priv;

    ssize_t unused = write(p->self_pipe[1], "", 1); // write '\0'
    (void) unused;

    return 0;
}

static
void
register_funcs(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv;
    if (p->self_pipe[0] >= 0) {
        // L: table
        lua_pushlightuserdata(L, pd); // L: table bd
        lua_pushcclosure(L, l_wake_up, 1); // L: table pd l_wake_up
        ls_lua_rawsetf(L, "wake_up"); // L: table
    }
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
    static const size_t BUF_SZ = 32;
    char *buf = LS_XNEW(char, BUF_SZ);

    int rcard = -1;
    while (snd_card_next(&rcard) >= 0 && rcard >= 0) {
        snprintf(buf, BUF_SZ, "hw:%d", rcard);
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

    snd_mixer_t *mixer;
    bool mixer_opened = false;
    snd_mixer_selem_id_t *sid;
    bool sid_alloced = false;
    char *realname = NULL;
    PollFdSet pollfds = (p->self_pipe[0] >= 0)
        ? pollfd_set_new((struct pollfd [1]) {{.fd = p->self_pipe[0], .events = POLLIN}}, 1)
        : pollfd_set_new(NULL, 0);

    if (!(realname = xalloc_card_realname(p->card))) {
        realname = ls_xstrdup(p->card);
    }

    // We do not check for /r_ == -EINTR/ as the only function that can
    // return /-EINTR/ is /snd_mixer_wait/, because it uses one of the
    // multiplexing interfaces mentioned below:
    //
    // http://man7.org/linux/man-pages/man7/signal.7.html
    //
    // > The following interfaces are never restarted after being interrupted
    // > by a signal handler, regardless of the use of SA_RESTART; they always
    // > fail with the error EINTR when interrupted by a signal handler:
    // [...]
    // >     * File descriptor multiplexing interfaces: epoll_wait(2),
    // >       epoll_pwait(2), poll(2), ppoll(2), select(2), and pselect(2).
#define ALSA_CALL(Func_, ...) \
    do { \
        int r_ = Func_(__VA_ARGS__); \
        if (r_ < 0) { \
            LS_FATALF(pd, "%s: %s", #Func_, snd_strerror(r_)); \
            goto error; \
        } \
    } while (0)

    ALSA_CALL(snd_mixer_open, &mixer, 0);
    mixer_opened = true;

    ALSA_CALL(snd_mixer_attach, mixer, realname);
    ALSA_CALL(snd_mixer_selem_register, mixer, NULL, NULL);
    ALSA_CALL(snd_mixer_load, mixer);

    ALSA_CALL(snd_mixer_selem_id_malloc, &sid);
    sid_alloced = true;

    snd_mixer_selem_id_set_name(sid, p->channel);
    snd_mixer_elem_t *elem = snd_mixer_find_selem(mixer, sid);
    if (!elem) {
        LS_FATALF(pd, "can't find channel '%s'", p->channel);
        goto error;
    }

    int (*get_range)(snd_mixer_elem_t *, long *, long *) =
        p->capture ? (p->in_db ? snd_mixer_selem_get_capture_dB_range
                               : snd_mixer_selem_get_capture_volume_range)
                   : (p->in_db ? snd_mixer_selem_get_playback_dB_range
                               : snd_mixer_selem_get_playback_volume_range);

    int (*get_cur)(snd_mixer_elem_t *, snd_mixer_selem_channel_id_t, long *) =
        p->capture ? (p->in_db ? snd_mixer_selem_get_capture_dB
                               : snd_mixer_selem_get_capture_volume)
                   : (p->in_db ? snd_mixer_selem_get_playback_dB
                               : snd_mixer_selem_get_playback_volume);

    int (*get_switch)(snd_mixer_elem_t *, snd_mixer_selem_channel_id_t, int *) =
        p->capture ? snd_mixer_selem_get_capture_switch
                   : snd_mixer_selem_get_playback_switch;

    ret = true;

    while (1) {
        lua_State *L = funcs.call_begin(pd->userdata);
        lua_createtable(L, 0, 2); // L: table

        lua_createtable(L, 0, 3); // L: table table
        long pmin, pmax;
        if (get_range(elem, &pmin, &pmax) >= 0) {
            lua_pushnumber(L, pmin); // L: table table pmin
            lua_setfield(L, -2, "min"); // L: table table
            lua_pushnumber(L, pmax); // L: table table pmax
            lua_setfield(L, -2, "max"); // L: table table
        }
        long pcur;
        if (get_cur(elem, 0, &pcur) >= 0) {
            lua_pushnumber(L, pcur); // L: table table pcur
            lua_setfield(L, -2, "cur"); // L: table table
        }
        lua_setfield(L, -2, "vol"); // L: table

        int notmute;
        if (get_switch(elem, 0, &notmute) >= 0) {
            lua_pushboolean(L, !notmute); // L: table !notmute
            lua_setfield(L, -2, "mute"); // L: table
        }

        funcs.call_end(pd->userdata);

        pollfd_set_resize(&pollfds, pollfds.nprefix + snd_mixer_poll_descriptors_count(mixer));
        ALSA_CALL(snd_mixer_poll_descriptors,
            mixer,
            pollfds.data + pollfds.nprefix,
            pollfds.size - pollfds.nprefix);

        int r;
        while ((r = poll(pollfds.data, pollfds.size, -1)) < 0 && errno == EINTR) {}
        if (r < 0) {
            LS_FATALF(pd, "poll: %s", ls_strerror_onstack(errno));
            goto error;
        }
        if (pollfds.nprefix && (pollfds.data[0].revents & POLLIN)) {
            ssize_t unused = read(p->self_pipe[0], (char [1]) {'\0'}, 1);
            (void) unused;
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
    if (sid_alloced) {
        snd_mixer_selem_id_free(sid);
    }
    if (mixer_opened) {
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
