/*
 * Copyright (C) 2015-2020  luastatus developers
 *
 * This file is part of luastatus.
 *
 * luastatus is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * luastatus is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with luastatus.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <lua.h>
#include <alsa/asoundlib.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "libls/alloc_utils.h"
#include "libls/tls_ebuf.h"
#include "libls/poll_utils.h"
#include "libls/evloop_lfuncs.h"
#include "libls/time_utils.h"

typedef struct {
    char *card;
    char *channel;
    bool capture;
    bool in_db;
    double tmo;
    int pipefds[2];
} Priv;

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->card);
    free(p->channel);
    close(p->pipefds[0]);
    close(p->pipefds[1]);
    free(p);
}

static int init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .card = NULL,
        .channel = NULL,
        .capture = false,
        .in_db = false,
        .tmo = -1,
        .pipefds = {-1, -1},
    };
    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse card
    if (moon_visit_str(&mv, -1, "card", &p->card, NULL, true) < 0)
        goto mverror;
    if (!p->card)
        p->card = ls_xstrdup("default");

    // Parse channel
    if (moon_visit_str(&mv, -1, "channel", &p->channel, NULL, true) < 0)
        goto mverror;
    if (!p->channel)
        p->channel = ls_xstrdup("Master");

    // Parse capture
    if (moon_visit_bool(&mv, -1, "capture", &p->capture, true) < 0)
        goto mverror;

    // Parse in_db
    if (moon_visit_bool(&mv, -1, "in_db", &p->in_db, true) < 0)
        goto mverror;

    // Parse timeout
    if (moon_visit_num(&mv, -1, "timeout", &p->tmo, true) < 0)
        goto mverror;

    // Parse make_self_pipe
    bool mkpipe = false;
    if (moon_visit_bool(&mv, -1, "make_self_pipe", &mkpipe, true) < 0)
        goto mverror;
    if (mkpipe) {
        if (ls_self_pipe_open(p->pipefds) < 0) {
            LS_FATALF(pd, "ls_self_pipe_open: %s", ls_tls_strerror(errno));
            goto error;
        }
    }

    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static void register_funcs(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv;
    // L: table
    ls_self_pipe_push_luafunc(p->pipefds, L); // L: table func
    lua_setfield(L, -2, "wake_up"); // L: table
}

static bool card_has_nicename(
        const char *realname,
        snd_ctl_card_info_t *info,
        const char *nicename)
{
    snd_ctl_t *ctl;
    if (snd_ctl_open(&ctl, realname, 0) < 0)
        return false;

    bool r = false;
    if (snd_ctl_card_info(ctl, info) >= 0) {
        const char *name = snd_ctl_card_info_get_name(info);
        r = (strcmp(name, nicename) == 0);
    }

    snd_ctl_close(ctl);
    return r;
}

static char * xalloc_card_realname(const char *nicename)
{
    snd_ctl_card_info_t *info;
    if (snd_ctl_card_info_malloc(&info) < 0)
        ls_oom();

    enum { NBUF = 32 };
    char *buf = LS_XNEW(char, NBUF);

    int rcard = -1;
    while (snd_card_next(&rcard) >= 0 && rcard >= 0) {
        snprintf(buf, NBUF, "hw:%d", rcard);
        if (card_has_nicename(buf, info, nicename))
            goto cleanup;
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

static GetVolFuncs select_gv_funcs(bool capture, bool in_db)
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

static void push_vol_info(lua_State *L, snd_mixer_elem_t *elem, GetVolFuncs gv_funcs)
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

static inline PollFdSet pollfd_set_new(struct pollfd prefix)
{
    if (prefix.fd >= 0) {
        return (PollFdSet) {
            .data = ls_xmemdup(&prefix, sizeof(struct pollfd)),
            .size = 1,
            .nprefix = 1,
        };
    } else {
        return (PollFdSet) {
            .data = NULL,
            .size = 0,
            .nprefix = 0,
        };
    }
}

static inline void pollfd_set_resize(PollFdSet *s, size_t n)
{
    if (s->size != n) {
        s->data = ls_xrealloc(s->data, n, sizeof(struct pollfd));
        s->size = n;
    }
}

static inline void pollfd_set_free(PollFdSet s)
{
    free(s.data);
}

static bool iteration(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;
    bool ret = false;

    snd_mixer_t *mixer = NULL;
    snd_mixer_selem_id_t *sid = NULL;
    char *realname = NULL;
    PollFdSet pollfds = pollfd_set_new((struct pollfd) {
        .fd = p->pipefds[0],
        .events = POLLIN,
    });

    if (!(realname = xalloc_card_realname(p->card))) {
        realname = ls_xstrdup(p->card);
    }

    int r;
    if ((r = snd_mixer_open(&mixer, 0)) < 0) {
        LS_FATALF(pd, "snd_mixer_open: %s", snd_strerror(r));
        goto error;
    }
    if ((r = snd_mixer_attach(mixer, realname)) < 0) {
        LS_FATALF(pd, "snd_mixer_attach: %s", snd_strerror(r));
        goto error;
    }
    if ((r = snd_mixer_selem_register(mixer, NULL, NULL)) < 0) {
        LS_FATALF(pd, "snd_mixer_selem_register: %s", snd_strerror(r));
        goto error;
    }
    if ((r = snd_mixer_load(mixer)) < 0) {
        LS_FATALF(pd, "snd_mixer_load: %s", snd_strerror(r));
        goto error;
    }
    if ((r = snd_mixer_selem_id_malloc(&sid)) < 0) {
        LS_FATALF(pd, "snd_mixer_selem_id_malloc: %s", snd_strerror(r));
        goto error;
    }
    snd_mixer_selem_id_set_name(sid, p->channel);
    snd_mixer_elem_t *elem = snd_mixer_find_selem(mixer, sid);
    if (!elem) {
        LS_FATALF(pd, "cannot find channel '%s'", p->channel);
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

        int nextrafds = snd_mixer_poll_descriptors_count(mixer);
        pollfd_set_resize(&pollfds, pollfds.nprefix + nextrafds);

        if ((r = snd_mixer_poll_descriptors(
                mixer,
                pollfds.data + pollfds.nprefix,
                pollfds.size - pollfds.nprefix)) < 0)
        {
            LS_FATALF(pd, "snd_mixer_poll_descriptors: %s", snd_strerror(r));
            goto error;
        }

        int nfds = ls_poll(pollfds.data, pollfds.size, p->tmo);
        if (nfds < 0) {
            LS_FATALF(pd, "poll: %s", ls_tls_strerror(errno));
            goto error;

        } else if (nfds == 0) {
            is_timeout = true;

        } else {
            is_timeout = false;

            if (pollfds.nprefix && (pollfds.data[0].revents & POLLIN)) {
                char c;
                ssize_t unused = read(p->pipefds[0], &c, 1);
                (void) unused;
            }
        }

        unsigned short revents;
        if ((r = snd_mixer_poll_descriptors_revents(
                mixer,
                pollfds.data + pollfds.nprefix,
                pollfds.size - pollfds.nprefix,
                &revents)) < 0)
        {
            LS_FATALF(pd, "snd_mixer_poll_descriptors_revents: %s", snd_strerror(r));
            goto error;
        }

        if (revents & (POLLERR | POLLNVAL)) {
            LS_FATALF(pd, "snd_mixer_poll_descriptors_revents() reported error condition");
            goto error;
        }
        if (revents & POLLIN) {
            if ((r = snd_mixer_handle_events(mixer)) < 0) {
                LS_FATALF(pd, "snd_mixer_handle_events: %s", snd_strerror(r));
                goto error;
            }
        }
    }
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

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    for (;;) {
        if (!iteration(pd, funcs)) {
            ls_sleep(5.0);
        }
    }
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .register_funcs = register_funcs,
    .run = run,
    .destroy = destroy,
};
