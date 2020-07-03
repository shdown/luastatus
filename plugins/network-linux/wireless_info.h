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

#ifndef wireless_info_h_
#define wireless_info_h_

#include <stdint.h>
#include <stdbool.h>
#include <linux/if_ether.h>

#define ESSID_MAX 32

enum {
    HAS_ESSID         = 1 << 0,
    HAS_SIGNAL_DBM    = 1 << 1,
    HAS_BITRATE       = 1 << 2,
    HAS_FREQUENCY     = 1 << 3,
};

typedef struct {
    int      flags;
    char     essid[ESSID_MAX + 1];
    uint8_t  bssid[ETH_ALEN];
    int      signal_dbm;
    unsigned bitrate;         // in units of 100 kbit/s
    double   frequency;
} WirelessInfo;

bool get_wireless_info(const char *iface, WirelessInfo *info);

#endif
