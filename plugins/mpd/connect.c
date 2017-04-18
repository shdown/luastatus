#include "connect.h"

#include "include/plugin_data.h"
#include "include/plugin_logf_macros.h"

#include <stddef.h>
#include <string.h>

#include <sys/un.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include "libls/errno_utils.h"
#include "libls/osdep.h"

int
socket_open(LuastatusPluginData *pd, const char *path)
{
    struct sockaddr_un saun;
    const size_t npath = strlen(path);
    if (npath + 1 > sizeof(saun.sun_path)) {
        LUASTATUS_ERRF(pd, "socket path is too long");
        return -1;
    }
    saun.sun_family = AF_UNIX;
    memcpy(saun.sun_path, path, npath + 1);
    int fd = ls_cloexec_socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        LS_WITH_ERRSTR(s, errno,
            LUASTATUS_ERRF(pd, "socket: %s", s);
        );
        return -1;
    }
    if (connect(fd, (const struct sockaddr *) &saun, sizeof(saun)) < 0) {
        LS_WITH_ERRSTR(s, errno,
            LUASTATUS_ERRF(pd, "connect: %s", s);
        );
        ls_close(fd);
        return -1;
    }
    return fd;
}

int
tcp_open(LuastatusPluginData *pd, const char *hostname, const char *service)
{
    struct addrinfo *ai = NULL;
    int fd = -1;

    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = 0,
        .ai_flags = AI_ADDRCONFIG,
    };

    int gai_r = getaddrinfo(hostname, service, &hints, &ai);
    if (gai_r) {
        if (gai_r == EAI_SYSTEM) {
            LS_WITH_ERRSTR(s, errno,
                LUASTATUS_ERRF(pd, "getaddrinfo: %s", s);
            );
        } else {
            LUASTATUS_ERRF(pd, "getaddrinfo: %s", gai_strerror(gai_r));
        }
        ai = NULL;
        goto cleanup;
    }

    for (struct addrinfo *pai = ai; pai; pai = pai->ai_next) {
        if ((fd = ls_cloexec_socket(pai->ai_family, pai->ai_socktype, pai->ai_protocol)) < 0) {
            LS_WITH_ERRSTR(s, errno,
                LUASTATUS_WARNF(pd, "(candiate) socket: %s", s);
            );
            continue;
        }
        if (connect(fd, pai->ai_addr, pai->ai_addrlen) < 0) {
            LS_WITH_ERRSTR(s, errno,
                LUASTATUS_WARNF(pd, "(candiate) connect: %s", s);
            );
            ls_close(fd);
            fd = -1;
            continue;
        }
        break;
    }
    if (fd < 0) {
        LUASTATUS_ERRF(pd, "can't connect to any of the candidates");
    }

cleanup:
    if (ai) {
        freeaddrinfo(ai);
    }
    return fd;
}
