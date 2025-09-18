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

#include "iface_type.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "libls/ls_io_utils.h"

bool is_wlan_iface(const char *iface)
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
    ls_close(fd);
    if (f) {
        fclose(f);
    }
    return ret;
}
