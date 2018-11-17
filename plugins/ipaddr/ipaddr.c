#ifndef __linux__
#   error "This plugin currently only supports Linux."
#endif
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
#include "include/plugin_utils.h"

#include "libls/alloc_utils.h"
#include "libls/cstring_utils.h"
#include "libls/osdep.h"

typedef struct {
    bool warn;
} Priv;

static
void
destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;
    free(p);
}

static
int
init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .warn = true,
    };

    PU_MAYBE_VISIT_BOOL("warn", NULL, b,
        p->warn = b;
    );
    return LUASTATUS_OK;

error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static
void
make_call(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    lua_State *L = funcs.call_begin(pd->userdata);
    lua_newtable(L); // L: table
    lua_newtable(L); // L: table ipv4
    lua_newtable(L); // L: table ipv4 ipv6
    struct ifaddrs *ifaddr = NULL;

    if (getifaddrs(&ifaddr) < 0) {
        LS_ERRF(pd, "getifaddrs: %s", ls_strerror_onstack(errno));
        ifaddr = NULL;
        goto done;
    }

    for (struct ifaddrs *cur = ifaddr; cur; cur = cur->ifa_next) {
        if (!cur->ifa_addr) {
            continue;
        }
        int family = cur->ifa_addr->sa_family;
        if (family != AF_INET && family != AF_INET6) {
            continue;
        }
        char host[1025];
        int r = getnameinfo(
            cur->ifa_addr,
            family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
            host, sizeof(host),
            NULL, 0, NI_NUMERICHOST);
        if (r) {
            Priv *p = pd->priv;
            if (p->warn) {
                LS_WARNF(pd, "getnameinfo [%s]: %s", cur->ifa_name, gai_strerror(r));
            }
            continue;
        }
        lua_pushstring(L, host); // L: table ipv4 ipv6 host
        lua_setfield(L, family == AF_INET ? -3 : -2, cur->ifa_name); // L: table ipv4 ipv6
    }

done:
    if (ifaddr) {
        freeifaddrs(ifaddr);
    }
    // L: table ipv4 ipv6
    lua_setfield(L, -3, "ipv6"); // L: table ipv4
    lua_setfield(L, -2, "ipv4"); // L: table
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

    // netlink(7) says "8192 to avoid message truncation on platforms with page size > 4096"
    char buf[8192];
    while (1) {
        make_call(pd, funcs);

        struct iovec iov = {buf, sizeof(buf)};
        struct sockaddr_nl sender;
        struct msghdr msg = {&sender, sizeof(sender), &iov, 1, NULL, 0, 0};
        ssize_t len = recvmsg(fd, &msg, 0);
        if (len < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (errno == ENOBUFS) {
                ret = true;
                LS_WARNF(pd, "ENOBUFS - kernel's socket buffer is full");
            } else {
                LS_FATALF(pd, "recvmsg: %s", ls_strerror_onstack(errno));
            }
            goto error;
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
        // some non-fatal error occurred; sleep for 5 seconds and restart the loop
        nanosleep((struct timespec [1]) {{.tv_sec = 5}}, NULL);
        LS_INFOF(pd, "resynchronizing");
    }
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
