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

#include "server.h"
#include <libwebsockets.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "buffer.h"
#include "common.h"

typedef struct {
    BeforeRequestCallback before_req_cb;
    WriteBodyCallback write_body_cb;
    AfterRequestCallback after_req_cb;
    ReadyCallback ready_cb;
    void *ud;
} Callbacks;

static Callbacks callbacks;

typedef struct {
    uint64_t cookie;
    Buffer buf;
    bool done_writing;
} MyContext;

static uint64_t next_cookie;

static void my_context_new(MyContext *my_ctx)
{
    *my_ctx = (MyContext) {
        .cookie = next_cookie++,
        .buf = BUFFER_STATIC_INIT,
        .done_writing = false,
    };
}

static int my_context_before_request(
    MyContext *my_ctx,
    const char *path,
    bool is_method_post,
    char **out_mime_type)
{
    return callbacks.before_req_cb(
        my_ctx->cookie,
        path,
        is_method_post,
        out_mime_type,
        callbacks.ud
    );
}

static void my_context_add_chunk(MyContext *ctx, const char *chunk, size_t len)
{
    buffer_append(&ctx->buf, chunk, len);
}

static char *my_context_write_body(
    MyContext *my_ctx,
    size_t *out_len)
{
    return callbacks.write_body_cb(
        my_ctx->cookie,
        my_ctx->buf.data,
        my_ctx->buf.size,
        out_len,
        callbacks.ud
    );
}

static void my_context_destroy(MyContext *my_ctx)
{
    buffer_destroy(&my_ctx->buf);
    my_ctx->buf = (Buffer) BUFFER_STATIC_INIT;
}

static int my_callback(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *userdata,
    void *in,
    size_t len)
{
    MyContext *my_ctx = userdata;

    uint8_t buf[LWS_PRE + 512];
    uint8_t *start = buf + LWS_PRE;
    uint8_t *end = buf + sizeof(buf) - 1;
    uint8_t *p = start;

    if (reason == LWS_CALLBACK_HTTP) {
        my_context_new(my_ctx);
        char *mime_type;
        int status = my_context_before_request(
            my_ctx,
            /*path=*/ in,
            /*is_method_post=*/ lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI) > 0,
            /*out_mime_type=*/ &mime_type
        );

        bool is_ok = true;
        if (lws_add_http_common_headers(
                wsi,
                status,
                mime_type,
                LWS_ILLEGAL_HTTP_CONTENT_LEN,
                &p,
                end) < 0)
        {
            is_ok = false;
        }
        if (lws_finalize_write_http_header(wsi, start, &p, end)) {
            is_ok = false;
        }

        free(mime_type);

        if (is_ok) {
            lws_callback_on_writable(wsi);
        }
        return is_ok ? 0 : -1;

    } else if (reason == LWS_CALLBACK_HTTP_BODY) {
        my_context_add_chunk(my_ctx, in, len);
        return 0;

    } else if (reason == LWS_CALLBACK_HTTP_BODY_COMPLETION) {
        lws_callback_on_writable(wsi);
        return 0;

    } else if (reason == LWS_CALLBACK_HTTP_WRITEABLE) {
        if (!my_ctx) {
            return -1;
        }
        if (my_ctx->done_writing) {
            return 0;
        }
        my_ctx->done_writing = true;

        size_t nbody;
        char *body = my_context_write_body(my_ctx, &nbody);
        if (nbody > INT_MAX) {
            panic("response body is too large");
        }

        int write_rc = lws_write(wsi, (uint8_t *) body, nbody, LWS_WRITE_HTTP_FINAL);

        my_context_destroy(my_ctx);
        free(body);

        if (write_rc != (int) nbody) {
            return -1;
        }
        if (lws_http_transaction_completed(wsi)) {
            return -1;
        }
        return 0;

    } else if (reason == LWS_CALLBACK_CLOSED_HTTP) {
        my_context_destroy(my_ctx);
        callbacks.after_req_cb(callbacks.ud);
        return 0;

    } else {
        return 0;
    }
}

static struct lws_protocols protocols[] = {
    {"http", my_callback, sizeof(MyContext), 0, 0, NULL, 0},
    {NULL, NULL, 0, 0, 0, NULL, 0},
};

static const struct lws_http_mount mount = {
    .mount_next = NULL,
    .mountpoint = "/",
    .mountpoint_len = 1,
    .origin = NULL,
    .def = NULL,
    .protocol = "http",
    .cgienv = NULL,
    .extra_mimetypes = NULL,
    .interpret = NULL,
    .cgi_timeout = 0,
    .cache_max_age = 0,
    .auth_mask = 0,
    .cache_reusable = 0,
    .cache_revalidate = 0,
    .cache_intermediaries = 0,
    .origin_protocol = LWSMPRO_CALLBACK,
    .basic_auth_login_file = NULL,
};

bool run_server(
    int port,
    BeforeRequestCallback before_req_cb,
    WriteBodyCallback write_body_cb,
    AfterRequestCallback after_req_cb,
    ReadyCallback ready_cb,
    void *ud)
{
    callbacks = (Callbacks) {
        .before_req_cb = before_req_cb,
        .write_body_cb = write_body_cb,
        .after_req_cb = after_req_cb,
        .ready_cb = ready_cb,
        .ud = ud,
    };

    signal(SIGPIPE, SIG_IGN);

    struct lws_context_creation_info info = {
        .port = port,
        .protocols = protocols,
        .mounts = &mount,
        .options = LWS_SERVER_OPTION_DISABLE_IPV6,
    };

    lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE, NULL);

    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "lws_create_context() failed\n");
        return false;
    }

    bool first = true;

    while (lws_service(context, 1000) >= 0) {
        if (first) {
            callbacks.ready_cb(callbacks.ud);
            first = false;
        }
    }

    // unreachable

    lws_context_destroy(context);
    return true;
}
