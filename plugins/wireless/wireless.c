#ifndef __linux__
#   error "This plugin currently only supports Linux."
#endif

#include <lua.h>
#include <asm/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "include/plugin_v1.h"
#include "include/plugin_utils.h"
#include "include/sayf_macros.h"

#include "libls/alloc_utils.h"
#include "libls/osdep.h"
#include "libls/cstring_utils.h"
#include "libls/time_utils.h"

#include "wireless_info.h"
#include "detect_iface.h"

// This is to make gcc happy.
#if EAGAIN == EWOULDBLOCK
#   define IS_EAGAIN(E_) ((E_) == EAGAIN)
#else
#   define IS_EAGAIN(E_) ((E_) == EAGAIN || (E_) == EWOULDBLOCK)
#endif

typedef struct {
    char *iface;
    struct timeval timeout;
    char *detected_iface;
} Priv;

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p->iface);
    free(p);
    free(p->detected_iface);
}

static
int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .iface = NULL,
        .timeout = ls_timeval_invalid,
        .detected_iface = NULL,
    };

    PU_MAYBE_VISIT_STR("iface", NULL, s,
        p->iface = ls_xstrdup(s);
    );

    PU_MAYBE_VISIT_NUM("timeout", NULL, n,
        if (ls_timeval_is_invalid(p->timeout = ls_timeval_from_seconds(n)) && n > 0) {
            LS_FATALF(pd, "'timeout' is invalid");
            goto error;
        }
    );

    return LUASTATUS_OK;

error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static
void
report_status(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs, const char *what)
{
    lua_State *L = funcs.call_begin(pd->userdata);
    // L: ?
    lua_newtable(L); // L: ? table
    lua_pushstring(L, what); // L: ? table str
    lua_setfield(L, -2, "what"); // L: ? table
    funcs.call_end(pd->userdata);
}

static
bool
query(LuastatusPluginData *pd, WirelessInfo *info)
{
    Priv *p = pd->priv;
    if (p->iface) {
        return get_wireless_info(p->iface, info);
    } else {
        if (p->detected_iface && get_wireless_info(p->detected_iface, info)) {
            return true;
        }
        if (!(p->detected_iface = detect_wlan_iface())) {
            return false;
        }
        LS_DEBUGF(pd, "detected wlan interface: %s", p->detected_iface);
        return get_wireless_info(p->detected_iface, info);
    }
}

static
void
make_call(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    WirelessInfo info;
    if (!query(pd, &info)) {
        report_status(pd, funcs, "error");
        return;
    }

    lua_State *L = funcs.call_begin(pd->userdata);
    // L: ?
    lua_newtable(L); // L: ? table
    lua_pushstring(L, "update"); // L: ? table str
    lua_setfield(L, -2, "what"); // L: ? table

    if (info.flags & HAS_ESSID) {
        lua_pushstring(L, info.essid); // L: ? table str
        lua_setfield(L, -2, "ssid"); // L: ? table
    }
    if (info.flags & HAS_SIGNAL_PERC) {
        lua_pushinteger(L, info.signal_perc); // L: ? table int
        lua_setfield(L, -2, "signal_percent"); // L: ? table
    }
    if (info.flags & HAS_SIGNAL_DBM) {
        lua_pushnumber(L, info.signal_dbm); // L: ? table number
        lua_setfield(L, -2, "signal_dbm"); // L: ? table
    }
    if (info.flags & HAS_FREQUENCY) {
        lua_pushnumber(L, info.frequency); // L: ? table number
        lua_setfield(L, -2, "frequency"); // L: ? table
    }
    if (info.flags & HAS_BITRATE) {
        lua_pushnumber(L, info.bitrate); // L: ? table number
        lua_setfield(L, -2, "bitrate"); // L: ? table
    }

    funcs.call_end(pd->userdata);
}

static
bool
interact(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    bool ret = false;
    int fd = ls_cloexec_socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (fd < 0) {
        LS_FATALF(pd, "socket: %s", ls_strerror_onstack(errno));
        goto error;
    }
    struct sockaddr_nl sa = {
        .nl_family = AF_NETLINK,
        .nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR,
    };
    if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        LS_FATALF(pd, "bind: %s", ls_strerror_onstack(errno));
        goto error;
    }

    const struct timeval timeout = ((Priv *) pd->priv)->timeout;
    if (!ls_timeval_is_invalid(timeout)) {
        if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            LS_WARNF(pd, "setsockopt: %s", ls_strerror_onstack(errno));
        }
    }

    // netlink(7) says "8192 to avoid message truncation on platforms with page size > 4096"
    char buf[8192];
    while (1) {
        make_call(pd, funcs);

        struct iovec iov = {buf, sizeof(buf)};
        struct sockaddr_nl sender;
        struct msghdr msg = {&sender, sizeof(sender), &iov, 1, NULL, 0, 0};
        ssize_t len = recvmsg(fd, &msg, 0);
        if (len < 0) {
            if (errno == EINTR || IS_EAGAIN(errno)) {
                continue;
            } else if (errno == ENOBUFS) {
                ret = true;
                LS_WARNF(pd, "ENOBUFS - kernel's socket buffer is full");
                goto error;
            } else {
                LS_FATALF(pd, "recvmsg: %s", ls_strerror_onstack(errno));
                goto error;
            }
        }

        for (struct nlmsghdr *nh = (struct nlmsghdr *) buf;
             NLMSG_OK(nh, len);
             nh = NLMSG_NEXT(nh, len))
        {
            if (nh->nlmsg_type == NLMSG_DONE) {
                // end of multipart message
                break;
            }
            if (nh->nlmsg_type == NLMSG_ERROR) {
                char *payload = (char *) (nh + 1);
                struct nlmsgerr *e = (struct nlmsgerr *) payload;
                int errnum = e->error;
                if (errnum) {
                    LS_ERRF(pd, "netlink error: %s", ls_strerror_onstack(-errnum));
                    ret = true;
                    goto error;
                } else {
                    LS_WARNF(pd, "unexpected ACK - what's going on?");
                    continue;
                }
            }
            // we don't care about the message
        }
    }

error:
    close(fd);
    return ret;
}

static
void
run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    while (interact(pd, funcs)) {
        // some non-fatal error occurred
        report_status(pd, funcs, "error");
        nanosleep((struct timespec [1]) {{.tv_sec = 5}}, NULL);
        LS_INFOF(pd, "resynchronizing");
    }
}

LuastatusPluginIface_v1 luastatus_plugin_iface_v1 = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
