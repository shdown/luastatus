/*
 * Copyright (C) 2021  luastatus developers
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

#include <sys/types.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

int main()
{
    struct ifaddrs *nets;
    if (getifaddrs(&nets) < 0) {
        perror("listnets: getifaddrs");
        return 1;
    }
    for (struct ifaddrs *p = nets; p; p = p->ifa_next) {
        if (!p->ifa_addr) {
            continue;
        }
        int family = p->ifa_addr->sa_family;
        if (family != AF_INET && family != AF_INET6) {
            continue;
        }
        char host[1025];
        int r = getnameinfo(
            p->ifa_addr,
            family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
            host, sizeof(host),
            NULL, 0, NI_NUMERICHOST);
        if (r) {
            if (r == EAI_SYSTEM) {
                perror("listnets: getnameinfo");
            } else {
                fprintf(stderr, "listnets: getnameinfo: %s\n", gai_strerror(r));
            }
            continue;
        }
        printf("%s %s %s\n", p->ifa_name, family == AF_INET ? "ipv4" : "ipv6", host);
    }
    freeifaddrs(nets);
    return 0;
}
