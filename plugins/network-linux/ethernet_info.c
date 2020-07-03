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

int get_ethernet_speed(int sockfd, const char *iface)
{
    struct ethtool_cmd ecmd = {.cmd = ETHTOOL_GSET};
    struct ifreq ifr = {.ifr_data = (caddr_t) &ecmd};

    size_t niface = strlen(iface);
    if (niface + 1 > sizeof(ifr.ifr_name))
        goto fail;
    memcpy(ifr.ifr_name, iface, niface + 1);

    if (ioctl(sockfd, SIOCETHTOOL, &ifr) < 0)
        goto fail;
    if (ecmd.speed == USHRT_MAX)
        goto fail;
    return ecmd.speed;

fail:
    return 0;
}
