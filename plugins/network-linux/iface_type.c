#include "iface_type.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

bool
is_wlan_iface(const char *iface)
{
    FILE *f = NULL;
    int fd = -1;
    char *line = NULL;
    size_t nline = 0;
    bool ret = false;

    char path[256];
    snprintf(path, sizeof(path), "/sys/class/net/%s/uevent", iface);
    if ((fd = open(path, O_RDONLY | O_CLOEXEC)) < 0) {
        goto done;
    }
    if (!(f = fdopen(fd, "r"))) {
        goto done;
    }
    fd = -1;
    while (getline(&line, &nline, f) > 0) {
        if (strcmp(line, "DEVTYPE=wlan\n") == 0) {
            ret = true;
            goto done;
        }
    }

done:
    free(line);
    close(fd);
    if (f) {
        fclose(f);
    }
    return ret;
}
