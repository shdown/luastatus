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
#include "include/plugin_utils.h"
#include "include/sayf_macros.h"

#include "libls/alloc_utils.h"
#include "libls/osdep.h"
#include "libls/cstring_utils.h"
#include "libls/time_utils.h"
#include "libls/strarr.h"

#include "string_set.h"
#include "wireless_info.h"
#include "ethernet_info.h"
#include "iface_type.h"

#if EAGAIN == EWOULDBLOCK
#   define IS_EAGAIN(E_) ((E_) == EAGAIN)
#else
#   define IS_EAGAIN(E_) ((E_) == EAGAIN || (E_) == EWOULDBLOCK)
#endif

enum {
    REPORT_IP       = 1 << 0,
    REPORT_WIRELESS = 1 << 1,
    REPORT_ETHERNET = 1 << 2,
};

typedef struct {
    int flags;
    struct timeval timeout;
    StringSet wlan_ifaces;
    int eth_sockfd;
} Priv;

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    string_set_destroy(p->wlan_ifaces);
    close(p->eth_sockfd);
    free(p);
}

static
void
modify_bit(int *mask, int bit, bool value)
{
    if (value) {
        *mask |= bit;
    } else {
        *mask &= ~bit;
    }
}

static
int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .flags = REPORT_IP,
        .timeout = ls_timeval_invalid,
        .wlan_ifaces = string_set_new(),
        .eth_sockfd = -1,
    };

    PU_MAYBE_VISIT_BOOL("ip", NULL, b,
        modify_bit(&p->flags, REPORT_IP, b);
    );

    PU_MAYBE_VISIT_BOOL("wireless", NULL, b,
        modify_bit(&p->flags, REPORT_WIRELESS, b);
    );

    PU_MAYBE_VISIT_BOOL("ethernet", NULL, b,
        modify_bit(&p->flags, REPORT_ETHERNET, b);
    );

    PU_MAYBE_VISIT_NUM("timeout", NULL, n,
        if (ls_timeval_is_invalid(p->timeout = ls_timeval_from_seconds(n)) && n > 0) {
            LS_FATALF(pd, "'timeout' is invalid");
            goto error;
        }
    );

    if (p->flags & REPORT_ETHERNET) {
        p->eth_sockfd = ls_cloexec_socket(AF_INET, SOCK_DGRAM, 0);
        if (p->eth_sockfd < 0) {
            LS_WARNF(pd, "ls_cloexec_socket: %s", ls_strerror_onstack(errno));
        }
    }

    return LUASTATUS_OK;

error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static
void
report_error(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    lua_State *L = funcs.call_begin(pd->userdata);
    // L: ?
    lua_pushnil(L); // L: ? nil
    funcs.call_end(pd->userdata);
}

static
void
inject_ip_info(lua_State *L, struct ifaddrs *addr)
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
    lua_pushstring(L, host); // L: ? ifacetbl ip
    lua_setfield(L, -2, family == AF_INET ? "ipv4" : "ipv6"); // L: ? ifacetbl
}

static
void
inject_wireless_info(lua_State *L, struct ifaddrs *addr)
{
    WirelessInfo info;
    if (!get_wireless_info(addr->ifa_name, &info)) {
        return;
    }

    // L: ? ifacetbl
    lua_newtable(L); // L: ? ifacetbl table
    if (info.flags & HAS_ESSID) {
        lua_pushstring(L, info.essid); // L: ? ifacetbl table str
        lua_setfield(L, -2, "ssid"); // L: ? ifacetbl table
    }
    if (info.flags & HAS_SIGNAL_PERC) {
        lua_pushinteger(L, info.signal_perc); // L: ? ifacetbl table int
        lua_setfield(L, -2, "signal_percent"); // L: ? ifacetbl table
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

static
void
inject_ethernet_info(lua_State *L, struct ifaddrs *addr, int sockfd)
{
    const int speed = get_ethernet_speed(sockfd, addr->ifa_name);
    if (!speed) {
        return;
    }
    // L: ? ifacetbl
    lua_newtable(L); // L: ? ifacetbl table

    lua_pushnumber(L, speed); // L: ? ifacetbl table number
    lua_setfield(L, -2, "speed"); // L: ? ifacetbl table

    lua_setfield(L, -2, "ethernet"); // L: ? ifacetbl
}

static
void
make_call(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs, bool timeout)
{
    struct ifaddrs *ifaddr;
    if (getifaddrs(&ifaddr) < 0) {
        LS_ERRF(pd, "getifaddrs: %s", ls_strerror_onstack(errno));
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

            if (p->flags & REPORT_WIRELESS) {
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

            if (p->flags & REPORT_ETHERNET) {
                inject_ethernet_info(L, cur, p->eth_sockfd); // L: ? table ifacetbl
            }
        }

        if (p->flags & REPORT_IP) {
            inject_ip_info(L, cur); // L: ? table ifacetbl
        }

        lua_pop(L, 1); // L: ? table
    }

    if (!timeout) {
        string_set_freeze(&p->wlan_ifaces);
    }

    freeifaddrs(ifaddr);
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
        .nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR,
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
        make_call(pd, funcs, false);

        struct iovec iov = {buf, sizeof(buf)};
        struct sockaddr_nl sender;
        struct msghdr msg = {&sender, sizeof(sender), &iov, 1, NULL, 0, 0};
        ssize_t len = recvmsg(fd, &msg, 0);
        if (len < 0) {
            if (errno == EINTR) {
                continue;
            } else if (IS_EAGAIN(errno)) {
                make_call(pd, funcs, true);
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
        report_error(pd, funcs);
        nanosleep((struct timespec [1]) {{.tv_sec = 5}}, NULL);
        LS_INFOF(pd, "resynchronizing");
    }
}

LuastatusPluginIface_v1 luastatus_plugin_iface_v1 = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
