#ifndef wireless_info_h_
#define wireless_info_h_

#include <stdint.h>
#include <stdbool.h>
#include <linux/if_ether.h>

#define ESSID_MAX 32

enum {
    HAS_ESSID        = 1 << 0,
    HAS_SIGNAL_PERC  = 1 << 1,
    HAS_SIGNAL_DBM   = 1 << 2,
    HAS_BITRATE      = 1 << 3,
    HAS_FREQUENCY    = 1 << 4,
};

typedef struct {
    int      flags;
    char     essid[ESSID_MAX + 1];
    uint8_t  bssid[ETH_ALEN];
    int      signal_perc;
    int32_t  signal_dbm;
    unsigned bitrate;         // in units of 100 kbit/s
    double   frequency;
} WirelessInfo;

bool
get_wireless_info(const char *iface, WirelessInfo *info);

#endif
