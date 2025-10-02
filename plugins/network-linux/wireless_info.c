/*
 * Copyright (C) 2015-2025  luastatus developers
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

#include "wireless_info.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <net/if.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <netlink/attr.h>
#include <netlink/handlers.h>
#include <netlink/msg.h>
#include <netlink/socket.h>
#include <linux/nl80211.h>
#include <linux/if_ether.h>
#include <linux/netlink.h>

// Adapted from i3status/src/print_wireless_info.c

static void find_ssid(uint8_t *ies, uint32_t ies_len, uint8_t **ssid, uint32_t *ssid_len)
{
    enum { WLAN_EID_SSID = 0 };

    *ssid = NULL;
    *ssid_len = 0;

    while (ies_len > 2 && ies[0] != WLAN_EID_SSID) {
        ies_len -= ies[1] + 2;
        ies += ies[1] + 2;
    }

    if (ies_len < 2)
        return;

    if (ies_len < ((uint32_t) ies[1]) + 2)
        return;

    *ssid_len = ies[1];
    *ssid = ies + 2;
}

static int gwi_sta_cb(struct nl_msg *msg, void *vud)
{
    WirelessInfo *info = vud;

    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *sinfo[NL80211_STA_INFO_MAX + 1];
    struct nlattr *rinfo[NL80211_RATE_INFO_MAX + 1];
    struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1] = {
        [NL80211_STA_INFO_RX_BITRATE] = {.type = NLA_NESTED},
        [NL80211_STA_INFO_SIGNAL] = {.type = NLA_U8},
    };
    struct nla_policy rate_policy[NL80211_RATE_INFO_MAX + 1] = {
        [NL80211_RATE_INFO_BITRATE] = {.type = NLA_U16},
    };

    if (nla_parse(
                tb,
                NL80211_ATTR_MAX,
                genlmsg_attrdata(gnlh, 0),
                genlmsg_attrlen(gnlh, 0),
                NULL)
            < 0)
    {
        return NL_SKIP;
    }

    if (!tb[NL80211_ATTR_STA_INFO])
        return NL_SKIP;

    if (nla_parse_nested(
                sinfo,
                NL80211_STA_INFO_MAX,
                tb[NL80211_ATTR_STA_INFO],
                stats_policy)
            < 0)
    {
        return NL_SKIP;
    }

    if (sinfo[NL80211_STA_INFO_SIGNAL]) {
        info->flags |= HAS_SIGNAL_DBM;
        info->signal_dbm = (int8_t) nla_get_u8(sinfo[NL80211_STA_INFO_SIGNAL]);
    }

    if (!sinfo[NL80211_STA_INFO_RX_BITRATE])
        return NL_SKIP;

    if (nla_parse_nested(
                rinfo,
                NL80211_RATE_INFO_MAX,
                sinfo[NL80211_STA_INFO_RX_BITRATE],
                rate_policy)
            < 0)
    {
        return NL_SKIP;
    }

    if (!rinfo[NL80211_RATE_INFO_BITRATE])
        return NL_SKIP;

    info->flags |= HAS_BITRATE;
    info->bitrate = nla_get_u16(rinfo[NL80211_RATE_INFO_BITRATE]);

    return NL_SKIP;
}

static int gwi_scan_cb(struct nl_msg *msg, void *vud)
{
    WirelessInfo *info = vud;
    struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *tb[NL80211_ATTR_MAX + 1];
    struct nlattr *bss[NL80211_BSS_MAX + 1];
    struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
        [NL80211_BSS_FREQUENCY] = {.type = NLA_U32},
        [NL80211_BSS_BSSID] = {.type = NLA_UNSPEC},
        [NL80211_BSS_INFORMATION_ELEMENTS] = {.type = NLA_UNSPEC},
        [NL80211_BSS_STATUS] = {.type = NLA_U32},
    };

    if (nla_parse(
                tb,
                NL80211_ATTR_MAX,
                genlmsg_attrdata(gnlh, 0),
                genlmsg_attrlen(gnlh, 0),
                NULL)
            < 0)
    {
        return NL_SKIP;
    }

    if (!tb[NL80211_ATTR_BSS])
        return NL_SKIP;

    if (nla_parse_nested(
                bss,
                NL80211_BSS_MAX,
                tb[NL80211_ATTR_BSS],
                bss_policy)
            < 0)
    {
        return NL_SKIP;
    }

    if (!bss[NL80211_BSS_STATUS])
        return NL_SKIP;

    uint32_t status = nla_get_u32(bss[NL80211_BSS_STATUS]);

    if (status != NL80211_BSS_STATUS_ASSOCIATED &&
        status != NL80211_BSS_STATUS_IBSS_JOINED)
    {
        return NL_SKIP;
    }

    if (!bss[NL80211_BSS_BSSID])
        return NL_SKIP;

    memcpy(info->bssid, nla_data(bss[NL80211_BSS_BSSID]), ETH_ALEN);

    if (bss[NL80211_BSS_FREQUENCY]) {
        info->flags |= HAS_FREQUENCY;
        info->frequency = ((double) nla_get_u32(bss[NL80211_BSS_FREQUENCY])) * 1e6;
    }

    if (bss[NL80211_BSS_INFORMATION_ELEMENTS]) {
        uint8_t *ssid;
        uint32_t ssid_len;

        find_ssid(
            nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]),
            nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]),
            &ssid,
            &ssid_len);

        if (ssid && ssid_len) {
            info->flags |= HAS_ESSID;
            snprintf(info->essid, sizeof(info->essid), "%.*s", (int) ssid_len, ssid);
        }
    }

    return NL_SKIP;
}

bool get_wireless_info(const char *iface, WirelessInfo *info)
{
    memset(info, 0, sizeof(WirelessInfo));
    bool ok = false;
    struct nl_sock *sk = NULL;
    struct nl_msg *msg = NULL;
    int r;

    if (!(sk = nl_socket_alloc()))
        goto done;

    if (genl_connect(sk) != 0)
        goto done;

    if (nl_socket_modify_cb(sk, NL_CB_VALID, NL_CB_CUSTOM, gwi_scan_cb, info) < 0)
        goto done;

    int nl80211_id = genl_ctrl_resolve(sk, "nl80211");
    if (nl80211_id < 0)
        goto done;


    unsigned ifidx = if_nametoindex(iface);
    if (ifidx == 0)
        goto done;

    if (!(msg = nlmsg_alloc()))
        goto done;

    if (!genlmsg_put(
            msg,
            NL_AUTO_PORT,
            NL_AUTO_SEQ,
            nl80211_id,
            0,
            NLM_F_DUMP,
            NL80211_CMD_GET_SCAN,
            0)
        )
    {
        goto done;
    }
    if (nla_put_u32(msg, NL80211_ATTR_IFINDEX, ifidx) < 0)
        goto done;

    r = nl_send_sync(sk, msg);
    msg = NULL;
    if (r < 0)
        goto done;

    if (nl_socket_modify_cb(sk, NL_CB_VALID, NL_CB_CUSTOM, gwi_sta_cb, info) < 0)
        goto done;

    if (!(msg = nlmsg_alloc()))
        goto done;

    if (!genlmsg_put(
            msg,
            NL_AUTO_PORT,
            NL_AUTO_SEQ,
            nl80211_id,
            0,
            NLM_F_DUMP,
            NL80211_CMD_GET_STATION,
            0)
        )
    {
        goto done;
    }

    if (nla_put_u32(msg, NL80211_ATTR_IFINDEX, ifidx) < 0)
        goto done;

    if (nla_put(msg, NL80211_ATTR_MAC, 6, info->bssid) < 0)
        goto done;

    r = nl_send_sync(sk, msg);
    msg = NULL;
    if (r < 0)
        goto done;

    ok = true;
done:
    if (msg)
        nlmsg_free(msg);
    if (sk)
        nl_socket_free(sk);
    return ok;
}
