/*
 * Copyright (C) 2026  luastatus developers
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

#include "ipc_client.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "include/sayf_macros.h"

#include "libls/ls_alloc_utils.h"
#include "libls/ls_panic.h"
#include "libls/ls_tls_ebuf.h"

#include "external_context.h"
#include "buffer.h"
#include "unixdom_open.h"
#include "compat_include_cjson.h"

#define MAGIC "i3-ipc"
// Subtract 1 because sizeof(literal) includes terminating '\0'.
#define MAGIC_LEN (sizeof(MAGIC) - 1)

struct IpcClient {
    ExternalContext ectx;

    int fd;
    Buffer buf;

    IpcClientCallback callback;
    void *callback_ud;

    bool debug;
};

IpcClient *ipc_client_create(
        ExternalContext ectx,
        const char *socket_path,
        IpcClientCallback callback,
        void *callback_ud,
        bool debug)
{
    int fd = unixdom_open(ectx, socket_path);
    if (fd < 0) {
        return NULL;
    }
    IpcClient *C = LS_XNEW(IpcClient, 1);
    *C = (IpcClient) {
        .ectx = ectx,

        .fd = fd,
        .buf = buffer_new(4096),

        .callback = callback,
        .callback_ud = callback_ud,

        .debug = debug,
    };
    return C;
}

static size_t dump_buf(IpcClient *C, char *dst, size_t ndst)
{
    const char *raw = buffer_ptr(&C->buf);

    size_t n = buffer_len(&C->buf);
    if (n > ndst) {
        n = ndst;
    }
    for (size_t i = 0; i < n; ++i) {
        unsigned char c = raw[i];
        if (c < 32 || c == 127) {
            c = '.';
        }
        dst[i] = c;
    }

    return n;
}

static inline void encode_header(char *dst, uint32_t len, uint32_t type)
{
    memcpy(dst, MAGIC, MAGIC_LEN);
    dst += MAGIC_LEN;

    memcpy(dst, &len, 4);
    dst += 4;

    memcpy(dst, &type, 4);
}

static inline bool decode_header(const char *src, uint32_t *out_len, uint32_t *out_type)
{
    if (memcmp(src, MAGIC, MAGIC_LEN) != 0) {
        return false;
    }
    src += MAGIC_LEN;

    memcpy(out_len, src, 4);
    src += 4;

    memcpy(out_type, src, 4);

    return true;
}

static inline bool do_full_read(IpcClient *C, char *dst, size_t ndst)
{
    size_t nread = 0;
    while (nread < ndst) {
        ssize_t r = read(C->fd, dst + nread, ndst - nread);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            goto error;
        } else if (r == 0) {
            goto eof;
        } else {
            nread += r;
        }
    }
    return true;

error:
    LS_FATALF(C->ectx, "read: %s", ls_tls_strerror(errno));
    return false;

eof:
    LS_FATALF(C->ectx, "read: got EOF");
    return false;
}

static inline bool do_full_write(IpcClient *C, const char *data, size_t ndata)
{
    size_t nwritten = 0;
    while (nwritten < ndata) {
        ssize_t w = write(C->fd, data + nwritten, ndata - nwritten);
        if (w < 0) {
            if (errno == EINTR) {
                continue;
            }
            goto error;
        }
        nwritten += w;
    }
    return true;

error:
    LS_FATALF(C->ectx, "write: %s", ls_tls_strerror(errno));
    return false;
}

static bool do_send_msg(IpcClient *C, const char *body, uint32_t type)
{
    size_t nbody = strlen(body);
    if (nbody >= UINT32_MAX) {
        LS_FATALF(C->ectx, "outgoing message is too long");
        return false;
    }

    char header[MAGIC_LEN + 4 + 4];
    encode_header(header, nbody, type);

    if (!do_full_write(C, header, sizeof(header))) {
        return false;
    }
    if (!do_full_write(C, body, nbody)) {
        return false;
    }

    if (C->debug) {
        LS_DEBUGF(C->ectx, "Send: [type %" PRIu32 "] %s", type, body);
    }

    return true;
}

static bool do_read_msg(IpcClient *C, uint32_t *out_type)
{
    char header[MAGIC_LEN + 4 + 4];

    if (!do_full_read(C, header, sizeof(header))) {
        return false;
    }

    uint32_t len;
    if (!decode_header(header, &len, out_type)) {
        LS_FATALF(C->ectx, "read message with wrong magic string");
        return false;
    }

    buffer_prepare(&C->buf, len);

    char *dst = buffer_ptr(&C->buf);
    if (!do_full_read(C, dst, len)) {
        return false;
    }

    if (C->debug) {
        char preview[256];
        int preview_len = dump_buf(C, preview, sizeof(preview));
        LS_DEBUGF(C->ectx, "Recv: [type %" PRIu32 "] %.*s", *out_type, preview_len, preview);
    }

    return true;
}

static void do_dump_bad(IpcClient *C, const char *msg_type_str)
{
    LS_ASSERT(msg_type_str != NULL);

    char preview[256];
    int preview_len = dump_buf(C, preview, sizeof(preview));

    LS_WARNF(C->ectx, "Preview of bad msg (type %s): %.*s", msg_type_str, preview_len, preview);
}

static inline cJSON *parse_json_from_buf(IpcClient *C)
{
    if (buffer_len(&C->buf) > (INT_MAX - 16)) {
        LS_ERRF(C->ectx, "message is too long to be parsed as JSON");
        return NULL;
    }

    const char *val = buffer_ptr(&C->buf);
    const char *err_ptr;

    cJSON *j = cJSON_ParseWithOpts(val, &err_ptr, /*require_null_terminate=*/ 1);
    if (!j) {
        LS_ERRF(C->ectx, "JSON parse error at byte %d", (int) (err_ptr - val));
    }
    return j;
}

static bool handle_reply_input(IpcClient *C)
{
    bool ok;

    cJSON *j = parse_json_from_buf(C);
    if (!j) {
        LS_WARNF(C->ectx, "read reply-to-input message, but can't parse it as JSON");
        ok = false;
        goto done;
    }

    if (!cJSON_IsArray(j)) {
        LS_WARNF(C->ectx, "read reply-to-input message, but body is not a list");
        ok = false;
        goto done;
    }

    ok = C->callback(j, C->callback_ud);

done:
    if (j) {
        cJSON_Delete(j);
    }
    return ok;
}

static bool handle_reply_subscribe(IpcClient *C)
{
    bool ok;

    cJSON *j = parse_json_from_buf(C);
    if (!j) {
        LS_FATALF(C->ectx, "read reply-to-subscribe message, but can't parse it as JSON");
        ok = false;
        goto done;
    }

    if (!cJSON_IsObject(j)) {
        LS_FATALF(C->ectx, "read reply-to-subscribe message, but body is not a dict");
        ok = false;
        goto done;
    }

    cJSON *j_success = cJSON_GetObjectItemCaseSensitive(j, "success");
    if (!j_success) {
        LS_FATALF(C->ectx, "read reply-to-subscribe message, but it has no 'success' entry");
        ok = false;
        goto done;
    }

    if (!cJSON_IsTrue(j_success)) {
        LS_FATALF(C->ectx, "read reply-to-subscribe message, but 'success' entry is not true");
        ok = false;
        goto done;
    }

    ok = true;
done:
    if (j) {
        cJSON_Delete(j);
    }
    return ok;
}

void ipc_client_interact(IpcClient *C)
{
    LS_ASSERT(C != NULL);

    if (!do_send_msg(C, "[\"input\"]", /*SUBSCRIBE*/ 2)) {
        return;
    }
    if (!do_send_msg(C, "", /*GET_INPUTS*/ 100)) {
        return;
    }

    for (;;) {
        uint32_t type;
        if (!do_read_msg(C, &type)) {
            return;
        }

        if (type == 0x80000015) {
            // event
            if (!do_send_msg(C, "", /*GET_INPUTS*/ 100)) {
                return;
            }
        } else if (type == 100) {
            // reply to GET_INPUTS
            if (!handle_reply_input(C)) {
                do_dump_bad(C, "input");
            }
        } else if (type == 2) {
            // reply to SUBSCRIBE
            if (!handle_reply_subscribe(C)) {
                do_dump_bad(C, "subscribe");
                return;
            }
        } else {
            LS_WARNF(C->ectx, "read message with unexpected type=%" PRIu32, type);
            do_dump_bad(C, "(unknown)");
        }
    }
}

void ipc_client_destroy(IpcClient *C)
{
    LS_ASSERT(C != NULL);

    close(C->fd);

    buffer_free(&C->buf);

    free(C);
}
