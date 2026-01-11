/*
 * Copyright (C) 2025  luastatus developers
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

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <libwebsockets.h>
#include "workmode.h"
#include "panic.h"

typedef struct {
    WorkMode *wm;
    int code;
    bool done_writing;
} MyRequestContext;

#define MAKE_ERROR_HTML(ErrorText_) \
    "<!doctype html>\n" \
    "<html lang=en>\n" \
    "<head>\n" \
    "<meta charset=utf-8>\n" \
    "<title>" ErrorText_ "</title>\n" \
    "</head>\n" \
    "<body>\n" \
    "<b>" ErrorText_ "</b>\n" \
    "</body>\n" \
    "</html>\n"

static const char *RESP_BODY_404 = MAKE_ERROR_HTML("404 Not Found");
static const char *RESP_BODY_405 = MAKE_ERROR_HTML("405 Method Not Allowed");

#undef MAKE_ERROR_HTML

typedef struct {
    char *path_without_leading_slashes;
    char *mime_type;
    int port;
    char *req_file;
    char *resp_string;
    char *resp_pattern;
    int max_requests;
    int notify_ready;
    int freeze_for;
} Options;

static Options options = {
    .path_without_leading_slashes = NULL,
    .mime_type = NULL,
    .port = -1,
    .req_file = NULL,
    .resp_string = NULL,
    .resp_pattern = NULL,
    .max_requests = -1,
    .notify_ready = 0,
    .freeze_for = 0,
};

static inline void ctx_destroy_wm_if_any(MyRequestContext *ctx)
{
    if (ctx->wm) {
        workmode_destroy(ctx->wm);
        ctx->wm = NULL;
    }
}

static void do_freeze(void)
{
    int ms = options.freeze_for;
    if (ms == 0) {
        return;
    }
    if (ms < 0) {
        fprintf(stderr, "httpserv: freezing forever.\n");
        for (;;) {
            pause();
        }
        MY_UNREACHABLE();
    }
    fprintf(stderr, "httpserv: freezing for %d ms.\n", ms);
    struct timespec ts = {
        .tv_sec = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000 * 1000,
    };
    struct timespec rem;
    while (nanosleep(&ts, &rem) < 0 && errno == EINTR) {
        ts = rem;
    }
    fprintf(stderr, "httpserv: unfreezing.\n");
}

static int my_callback(
    struct lws *wsi,
    enum lws_callback_reasons reason,
    void *userdata,
    void *in,
    size_t len)
{
    MyRequestContext *ctx = userdata;

    uint8_t buf[LWS_PRE + 512];
    uint8_t *start = buf + LWS_PRE;
    uint8_t *end = buf + sizeof(buf) - 1;
    uint8_t *p = start;

    if (reason == LWS_CALLBACK_HTTP) {
        *ctx = (MyRequestContext) {
            .wm = NULL,
            .code = 0,
            .done_writing = false,
        };

        int rc = 0;
        if (strcmp(in, options.path_without_leading_slashes) == 0) {
            // Path: OK
            if (lws_hdr_total_length(wsi, WSI_TOKEN_POST_URI) > 0) {
                // Method: OK
                ctx->code = 200;
                if (lws_add_http_common_headers(wsi, HTTP_STATUS_OK, options.mime_type, LWS_ILLEGAL_HTTP_CONTENT_LEN, &p, end)) {
                    rc = -1;
                }
            } else {
                ctx->code = 405;
                // Method: bad
                if (lws_add_http_common_headers(wsi, HTTP_STATUS_METHOD_NOT_ALLOWED, "text/html", strlen(RESP_BODY_405), &p, end)) {
                    rc = -1;
                }
            }
        } else {
            // Path: bad
            ctx->code = 404;
            if (lws_add_http_common_headers(wsi, HTTP_STATUS_NOT_FOUND, "text/html", strlen(RESP_BODY_404), &p, end)) {
                rc = -1;
            }
        }
        if (lws_finalize_write_http_header(wsi, start, &p, end)) {
            rc = -1;
        }

        if (rc == 0) {
            if (ctx->code == 200) {
                ctx->wm = workmode_new();
                MY_ASSERT(ctx->wm != NULL);
            } else {
                lws_callback_on_writable(wsi);
            }
        }
        return rc;

    } else if (reason == LWS_CALLBACK_HTTP_BODY) {
        if (ctx->code == 200) {
            workmode_request_chunk(ctx->wm, in, len);
        }
        return 0;

    } else if (reason == LWS_CALLBACK_HTTP_BODY_COMPLETION) {
        if (ctx->code == 200) {
            workmode_request_finish(ctx->wm);
        }
        lws_callback_on_writable(wsi);
        return 0;

    } else if (reason == LWS_CALLBACK_HTTP_WRITEABLE) {
        if (!ctx) {
            return -1;
        }
        if (ctx->done_writing) {
            return 0;
        }
        ctx->done_writing = true;

        const char *cur_resp;
        int cur_resp_len;
        switch (ctx->code) {
        case 200:
            cur_resp = workmode_get_response(ctx->wm, &cur_resp_len);
            break;
        case 404:
            cur_resp = RESP_BODY_404;
            cur_resp_len = strlen(RESP_BODY_404);
            break;
        case 405:
            cur_resp = RESP_BODY_405;
            cur_resp_len = strlen(RESP_BODY_405);
            break;
        default:
            MY_UNREACHABLE();
        }

        int write_rc = lws_write(wsi, (uint8_t *) cur_resp, cur_resp_len, LWS_WRITE_HTTP_FINAL);

        ctx_destroy_wm_if_any(ctx);

        if (write_rc != cur_resp_len) {
            return -1;
        }
        if (lws_http_transaction_completed(wsi)) {
            return -1;
        }
        return 0;

    } else if (reason == LWS_CALLBACK_CLOSED_HTTP) {
        ctx_destroy_wm_if_any(ctx);

        int *m = &options.max_requests;
        if (*m > 0 && !--(*m)) {
            fprintf(stderr, "httpserv: reached max_requests.\n");
            if (options.freeze_for) {
                do_freeze();
                *m = -1;
            } else {
                lws_cancel_service_pt(wsi);
            }
        }
        return 0;

    } else {
        return 0;
    }
}

static struct lws_protocols protocols[] = {
    {"http", my_callback, sizeof(MyRequestContext), 0, 0, NULL, 0},
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

enum {
    AS_TYPE_STR,
    AS_TYPE_INT,
    AS_TYPE_BOOL,
};

// First bit of "flags" part of ArgSpec::bits
enum {
    AS_FLAGS_FIRST_BIT = 1 << 10,
};
enum {
    AS_FLAG_WRITTEN_TO = AS_FLAGS_FIRST_BIT << 0,
    AS_FLAG_NEG_OK     = AS_FLAGS_FIRST_BIT << 1,
};

typedef struct {
    int is_required;
    const char *spelling;
    int bits;
    void *dst;
} ArgSpec;

static int parse_single_arg(const ArgSpec *AS, const char *v)
{
    int type = AS->bits & (AS_FLAGS_FIRST_BIT - 1);
    switch (type) {

    case AS_TYPE_STR:
        *((char **) AS->dst) = (char *) v;
        return 1;

    case AS_TYPE_INT:
        {
            int *dst = AS->dst;
            if (sscanf(v, "%d", dst) != 1) {
                fprintf(stderr, "Cannot parse value '%s', passed as option '%s', into int.\n", v, AS->spelling);
                return -1;
            }
            if (!(AS->bits & AS_FLAG_NEG_OK) && *dst < 0) {
                fprintf(stderr, "Value '%s', passed as option '%s', is negative.\n", v, AS->spelling);
                return -1;
            }
        }
        return 1;

    case AS_TYPE_BOOL:
        if (v[0] != '\0') {
            return 0;
        }
        *(int *) AS->dst = 1;
        return 1;

    default:
        MY_UNREACHABLE();
    }
}

static inline const char *strfollow(const char *str, const char *prefix)
{
    size_t nprefix = strlen(prefix);
    return strncmp(str, prefix, nprefix) == 0 ? str + nprefix : NULL;
}

static bool parse_args(ArgSpec *AS, int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        bool found = false;
        for (ArgSpec *p = AS; p->spelling; ++p) {
            const char *v = strfollow(arg, p->spelling);
            if (!v) {
                continue;
            }
            int rc = parse_single_arg(p, v);
            if (rc < 0) {
                return false;
            } else if (rc == 0) {
                continue;
            }
            p->bits |= AS_FLAG_WRITTEN_TO;
            found = true;
            break;
        }
        if (!found) {
            fprintf(stderr, "Unknown argument '%s'.\n", arg);
            return false;
        }
    }
    for (ArgSpec *p = AS; p->spelling; ++p) {
        if (p->is_required && !(p->bits & AS_FLAG_WRITTEN_TO)) {
            fprintf(stderr, "Required argument '%s' was not specified.\n", p->spelling);
            return false;
        }
    }
    return true;
}

static void print_usage_and_die(void)
{
    fprintf(stderr, "USAGE: httpserv PARAMETERS\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Required PARAMETERS:\n");
    fprintf(stderr, "  --path=...             | path to serve at\n");
    fprintf(stderr, "  --mime-type=...        | MIME type of response\n");
    fprintf(stderr, "  --port=...             | port to serve at\n");
    fprintf(stderr, "  --req-file=...         | file to write request body to\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "If should serve static content:\n");
    fprintf(stderr, "  --resp-string=...      | response body\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "If should perform dynamic replacement:\n");
    fprintf(stderr, "  --resp-pattern=...     | response template\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Optional PARAMETERS:\n");
    fprintf(stderr, "  --max-requests=...     | max # of requests to serve before terminating\n");
    fprintf(stderr, "  --notify-ready         | write 'httpserv_is_ready' line to req-file once ready\n");
    fprintf(stderr, "  --freeze-for=MILLIS    | freeze for MILLIS milliseconds instead of ...\n");
    fprintf(stderr, "                         |  ... exiting when max-requests is reached; -1 means ...\n");
    fprintf(stderr, "                         |  ... freezing forver.\n");
    exit(2);
}

int main(int argc, char **argv)
{
    char *path;

    ArgSpec arg_specs[] = {
        {1, "--path=",             AS_TYPE_STR,  &path},
        {1, "--mime-type=",        AS_TYPE_STR,  &options.mime_type},
        {1, "--port=",             AS_TYPE_INT,  &options.port},
        {1, "--req-file=",         AS_TYPE_STR,  &options.req_file},
        {0, "--resp-string=",      AS_TYPE_STR,  &options.resp_string},
        {0, "--resp-pattern=",     AS_TYPE_STR,  &options.resp_pattern},
        {0, "--max-requests=",     AS_TYPE_INT,  &options.max_requests},
        {0, "--notify-ready",      AS_TYPE_BOOL, &options.notify_ready},

        {0, "--freeze-for=",       AS_TYPE_INT | AS_FLAG_NEG_OK, &options.freeze_for},

        {0},
    };
    if (!parse_args(arg_specs, argc, argv)) {
        print_usage_and_die();
    }

    if (options.resp_string && options.resp_pattern) {
        fprintf(stderr, "Options '--resp-string=' and '--resp-pattern=' are mutually exclusive.\n");
        print_usage_and_die();
    }

    if (options.resp_string) {
        workmode_global_init_fxd(options.req_file, options.resp_string);
    } else {
        workmode_global_init_dyn(options.req_file, options.resp_pattern);
    }

    if (options.port > 65535) {
        fprintf(stderr, "Invalid port %d.\n", options.port);
        print_usage_and_die();
    }

    if (options.max_requests == 0) {
        options.max_requests = -1;
    }

    while (path[0] == '/') {
        ++path;
    }
    options.path_without_leading_slashes = path;

    signal(SIGPIPE, SIG_IGN);

    struct lws_context_creation_info info = {
        .port = options.port,
        .protocols = protocols,
        .mounts = &mount,
        .options = LWS_SERVER_OPTION_DISABLE_IPV6,
    };

    lws_set_log_level(LLL_USER | LLL_ERR | LLL_WARN | LLL_NOTICE, NULL);

    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        FATAL("lws_create_context() failed\n");
    }

    bool first = true;
    while (lws_service(context, 1000) >= 0) {
        if (first && options.notify_ready) {
            workmode_write_to_req_file("httpserv_is_ready\n");
        }
        first = false;

        if (options.max_requests == 0) {
            break;
        }
    }

    lws_context_destroy(context);

    workmode_global_uninit();

    return 0;
}
