#include <errno.h>
#include <lua.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"
#include "include/plugin_utils.h"

#include "libls/alloc_utils.h"

typedef struct {
    char *card;
    char *channel;
    bool capture;
    bool in_db;
} Priv;

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->card);
    free(p->channel);
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
    };

    PU_MAYBE_VISIT_STR("card", s,
        p->card = ls_xstrdup(s);
    );
    if (!p->card) {
        p->card = ls_xstrdup("default");
    }

    PU_MAYBE_VISIT_STR("channel", s,
        p->channel = ls_xstrdup(s);
    );
    if (!p->channel) {
        p->channel = ls_xstrdup("Master");
    }

    PU_MAYBE_VISIT_BOOL("capture", b,
        p->capture = b;
    );

    PU_MAYBE_VISIT_BOOL("in_db", b,
        p->in_db = b;
    );

    return LUASTATUS_OK;

error:
    destroy(pd);
    return LUASTATUS_ERR;
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

static
void
run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    snd_mixer_t *mixer;
    bool mixer_opened = false;
    snd_mixer_selem_id_t *sid;
    bool sid_alloced = false;
    char *realname = NULL;

    if (!(realname = xalloc_card_realname(p->card))) {
        realname = ls_xstrdup(p->card);
    }

    // Actually, the only function that can return /-EINTR/ is /snd_mixer_wait/,
    // because it uses one of the multiplexing interfaces mentioned below:
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
        int r_; \
        while ((r_ = Func_(__VA_ARGS__)) == -EINTR) {} \
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

    while (1) {
        lua_State *L = funcs.call_begin(pd->userdata);
        lua_newtable(L); // L: table
        lua_newtable(L); // L: table table
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
        ALSA_CALL(snd_mixer_wait, mixer, -1);
        ALSA_CALL(snd_mixer_handle_events, mixer);
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
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
