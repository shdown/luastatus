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
#include <asm/types.h>
#include <sys/types.h>
#include <sys/socket.h>
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
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "libls/alloc_utils.h"
#include "libls/io_utils.h"
#include "libls/osdep.h"
#include "libls/tls_ebuf.h"
#include "libls/time_utils.h"
#include "libls/strarr.h"

#include "string_set.h"
#include "wireless_info.h"
#include "ethernet_info.h"
#include "iface_type.h"

typedef struct {
    bool report_ip;
    bool report_wireless;
    bool report_ethernet;
    bool new_ip_fmt;
    double tmo;
    StringSet wlan_ifaces;
    int eth_sockfd;
} Priv;

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    string_set_destroy(p->wlan_ifaces);
    close(p->eth_sockfd);
    free(p);
}

static int init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .report_ip = true,
        .report_wireless = false,
        .report_ethernet = false,
        .new_ip_fmt = false,
        .tmo = -1,
        .wlan_ifaces = string_set_new(),
        .eth_sockfd = -1,
    };
    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse ip
    if (moon_visit_bool(&mv, -1, "ip", &p->report_ip, true) < 0)
        goto mverror;

    // Parse wireless
    if (moon_visit_bool(&mv, -1, "wireless", &p->report_wireless, true) < 0)
        goto mverror;

    // Parse ethernet
    if (moon_visit_bool(&mv, -1, "ethernet", &p->report_ethernet, true) < 0)
        goto mverror;

    // Parse new_ip_fmt
    if (moon_visit_bool(&mv, -1, "new_ip_fmt", &p->new_ip_fmt, true) < 0)
        goto mverror;

    // Parse timeout
    if (moon_visit_num(&mv, -1, "timeout", &p->tmo, true) < 0)
        goto mverror;

    // Open eth_sockfd if needed.
    if (p->report_ethernet) {
        p->eth_sockfd = ls_cloexec_socket(AF_INET, SOCK_DGRAM, 0);
        if (p->eth_sockfd < 0) {
            LS_WARNF(pd, "ls_cloexec_socket: %s", ls_tls_strerror(errno));
        }
    }

    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
//error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static void report_error(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    lua_State *L = funcs.call_begin(pd->userdata);
    // L: ?
    lua_pushnil(L); // L: ? nil
    funcs.call_end(pd->userdata);
}

static inline size_t get_the_bloody_length(lua_State *L, int i)
{
#if LUA_VERSION_NUM <= 501
    return lua_objlen(L, i);
#else
    return lua_rawlen(L, i);
#endif
}

static void inject_ip_info(lua_State *L, struct ifaddrs *addr, bool new_ip_fmt)
{
    // L: ? ifacetbl
    if (!addr->ifa_addr) {
        return;
    }
    int family = addr->ifa_addr->sa_family;
    if (family != AF_INET && family != AF_INET6) {
        return;
    }
    char host[1025];
    int r = getnameinfo(
        addr->ifa_addr,
        family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
        host, sizeof(host),
        NULL, 0, NI_NUMERICHOST);
    if (r) {
        return;
    }

    const char *k = family == AF_INET ? "ipv4" : "ipv6";
    if (new_ip_fmt) {
        lua_getfield(L, -1, k); // L: ? ifacetbl tbl
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1); // L: ? ifacetbl
            lua_newtable(L); // L: ? ifacetbl tbl
            lua_pushstring(L, host); // L: ? ifacetbl tbl str
            lua_rawseti(L, -2, 1); // L: ? ifacetbl tbl
            lua_setfield(L, -2, k); // L: ? ifacetbl
        } else {
            size_t n = get_the_bloody_length(L, -1); // L: ? ifacetbl tbl
            lua_pushstring(L, host); // L: ? ifacetbl tbl str
            lua_rawseti(L, -2, n + 1); // L: ? ifacetbl tbl
            lua_pop(L, 1); // L: ? ifacetbl
        }
    } else {
        lua_pushstring(L, host); // L: ? ifacetbl str
        lua_setfield(L, -2, k); // L: ? ifacetbl
    }
}

static void inject_wireless_info(lua_State *L, struct ifaddrs *addr)
{
    WirelessInfo info;
    if (!get_wireless_info(addr->ifa_name, &info)) {
        return;
    }

    // L: ? ifacetbl
    lua_createtable(L, 0, 4); // L: ? ifacetbl table
    if (info.flags & HAS_ESSID) {
        lua_pushstring(L, info.essid); // L: ? ifacetbl table str
        lua_setfield(L, -2, "ssid"); // L: ? ifacetbl table
    }
    if (info.flags & HAS_SIGNAL_DBM) {
        lua_pushnumber(L, info.signal_dbm); // L: ? ifacetbl table number
        lua_setfield(L, -2, "signal_dbm"); // L: ? ifacetbl table
    }
    if (info.flags & HAS_FREQUENCY) {
        lua_pushnumber(L, info.frequency); // L: ? ifacetbl table number
        lua_setfield(L, -2, "frequency"); // L: ? ifacetbl table
    }
    if (info.flags & HAS_BITRATE) {
        lua_pushnumber(L, info.bitrate); // L: ? ifacetbl table number
        lua_setfield(L, -2, "bitrate"); // L: ? ifacetbl table
    }
    lua_setfield(L, -2, "wireless"); // L: ? ifacetbl
}

static void inject_ethernet_info(lua_State *L, struct ifaddrs *addr, int sockfd)
{
    int speed = get_ethernet_speed(sockfd, addr->ifa_name);
    if (!speed) {
        return;
    }
    // L: ? ifacetbl
    lua_createtable(L, 0, 1); // L: ? ifacetbl table

    lua_pushnumber(L, speed); // L: ? ifacetbl table number
    lua_setfield(L, -2, "speed"); // L: ? ifacetbl table

    lua_setfield(L, -2, "ethernet"); // L: ? ifacetbl
}

static void make_call(
        LuastatusPluginData *pd,
        LuastatusPluginRunFuncs funcs,
        bool timeout)
{
    struct ifaddrs *ifaddr;
    if (getifaddrs(&ifaddr) < 0) {
        LS_ERRF(pd, "getifaddrs: %s", ls_tls_strerror(errno));
        report_error(pd, funcs);
        return;
    }

    lua_State *L = funcs.call_begin(pd->userdata); // L: ?
    lua_newtable(L); // L: ? table

    Priv *p = pd->priv;
    if (!timeout) {
        string_set_reset(&p->wlan_ifaces);
    }

    for (struct ifaddrs *cur = ifaddr; cur; cur = cur->ifa_next) {
        lua_getfield(L, -1, cur->ifa_name); // L: ? table ifacetbl
        if (lua_isnil(L, -1)) {
            lua_pop(L, 1); // L: ? table
            lua_newtable(L); // L: ? table ifacetbl
            lua_pushvalue(L, -1); // L: ? table ifacetbl ifacetbl
            lua_setfield(L, -3, cur->ifa_name); // L: ? table ifacetbl

            if (p->report_wireless) {
                bool is_wlan;
                if (timeout) {
                    is_wlan = string_set_contains(p->wlan_ifaces, cur->ifa_name);
                } else {
                    is_wlan = is_wlan_iface(cur->ifa_name);
                    if (is_wlan) {
                        string_set_add(&p->wlan_ifaces, cur->ifa_name);
                    }
                }
                if (is_wlan) {
                    inject_wireless_info(L, cur); // L: ? table ifacetbl
                }
            }

            if (p->report_ethernet) {
                inject_ethernet_info(L, cur, p->eth_sockfd); // L: ? table ifacetbl
            }
        }

        if (p->report_ip) {
            inject_ip_info(L, cur, p->new_ip_fmt); // L: ? table ifacetbl
        }

        lua_pop(L, 1); // L: ? table
    }

    if (!timeout) {
        string_set_freeze(&p->wlan_ifaces);
    }

    freeifaddrs(ifaddr);
    funcs.call_end(pd->userdata);
}

static void setup_sock_timeout(LuastatusPluginData *pd, int fd)
{
    Priv *p = pd->priv;
    if (p->tmo < 0)
        return;
    struct timeval tmo_tv = ls_tmo_to_tv(p->tmo);
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tmo_tv, sizeof(tmo_tv)) < 0) {
        LS_WARNF(pd, "setsockopt: %s", ls_tls_strerror(errno));
    }
}

static bool interact(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    // We allocate the buffer for netlink messages on the heap rather than on the stack, for two
    // reasons:
    //
    // 1. Alignment. The code in the netlink(7) man page seems to be wrong as it does not use
    // /__attribute__((aligned(...)))/, which is used, e.g., in the inotify(7) example.
    //
    // 2. Stack space is not free, and 8K is quite a large allocation for the stack.
    //
    // As for the buffer size, netlink(7) says "8192 to avoid message truncation on platforms with
    // page size > 4096".
    enum { NBUF = 8192 };

    bool ret = false;
    char *buf = LS_XNEW(char, NBUF);
    int fd = -1;

    fd = ls_cloexec_socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (fd < 0) {
        LS_FATALF(pd, "socket: %s", ls_tls_strerror(errno));
        goto error;
    }

    struct sockaddr_nl sa = {
        .nl_family = AF_NETLINK,
        .nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR,
    };
    if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        LS_FATALF(pd, "bind: %s", ls_tls_strerror(errno));
        goto error;
    }

    setup_sock_timeout(pd, fd);

    while (1) {
        make_call(pd, funcs, false);

        struct iovec iov = {.iov_base = buf, .iov_len = NBUF};
        struct msghdr msg = {.msg_iov = &iov, .msg_iovlen = 1};
        ssize_t len = recvmsg(fd, &msg, 0);
        if (len < 0) {
            if (errno == EINTR) {
                continue;
            } else if (LS_IS_EAGAIN(errno)) {
                make_call(pd, funcs, true);
                continue;
            } else if (errno == ENOBUFS) {
                ret = true;
                LS_WARNF(pd, "ENOBUFS - kernel's socket buffer is full");
                goto error;
            } else {
                LS_FATALF(pd, "recvmsg: %s", ls_tls_strerror(errno));
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
                if (nh->nlmsg_len < NLMSG_LENGTH(sizeof(struct nlmsgerr))) {
                    LS_ERRF(pd, "netlink error message truncated");
                    ret = true;
                    goto error;
                }
                struct nlmsgerr *e = NLMSG_DATA(nh);
                int errnum = e->error;
                if (errnum) {
                    LS_ERRF(pd, "netlink error: %s", ls_tls_strerror(-errnum));
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
    free(buf);
    close(fd);
    return ret;
}

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    while (interact(pd, funcs)) {
        // a non-fatal error occurred
        report_error(pd, funcs);
        ls_sleep(5.0);
        LS_INFOF(pd, "resynchronizing");
    }
}

LuastatusPluginIface_v1 luastatus_plugin_iface_v1 = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
