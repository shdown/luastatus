#define _GNU_SOURCE

#include "ethernet_info.h"

#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include "libls/osdep.h"

int
get_ethernet_speed(int sockfd, const char *iface)
{
    struct ethtool_cmd ecmd = {.cmd = ETHTOOL_GSET};
    struct ifreq ifr = {.ifr_data = (caddr_t) &ecmd};
    if (strlen(iface) + 1 > sizeof(ifr.ifr_name)) {
        goto fail;
    }
    memcpy(ifr.ifr_name, iface, strlen(iface) + 1);
    if (ioctl(sockfd, SIOCETHTOOL, &ifr) < 0) {
        goto fail;
    }
    if (ecmd.speed == USHRT_MAX) {
        goto fail;
    }
    return ecmd.speed;
fail:
    return 0;
}
