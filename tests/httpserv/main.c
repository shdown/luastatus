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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include "server.h"
#include "common.h"
#include "sleep_millis.h"
#include "argparser.h"

typedef struct {
    const char *expected_path;
    bool expected_method_is_post;
    int max_requests;
    int freeze_for;
} GlobalOptions;

static GlobalOptions global_options = {
    .max_requests = -1,
    .freeze_for = 0,
};

typedef struct {
    uint64_t cookie;
    int status;
    const char *status_text;
} Request;

typedef struct {
    Request *data;
    size_t size;
    size_t capacity;
} RequestList;

static RequestList request_list = {0};

Request *request_new(uint64_t cookie)
{
    Request req = {
        .cookie = cookie,
        .status = 0,
    };
    if (request_list.size == request_list.capacity) {
        request_list.data = x2realloc(request_list.data, &request_list.capacity, sizeof(Request));
    }
    Request *p_req = &request_list.data[request_list.size++];
    *p_req = req;
    return p_req;
}

static Request *request_find_or_die(uint64_t cookie)
{
    for (size_t i = 0; i < request_list.size; ++i) {
        if (request_list.data[i].cookie == cookie) {
            return &request_list.data[i];
        }
    }
    panic("cannot find request with given cookie");
}

static void request_free(Request *req)
{
    *req = request_list.data[request_list.size - 1];
    --request_list.size;
}

static int request_prepare_error(Request *req, int status, const char *status_text)
{
    req->status = status;
    req->status_text = status_text;
    return status;
}

static const char *strip_leading_slashes(const char *s)
{
    while (*s == '/') {
        ++s;
    }
    return s;
}

int my_before_req_cb(
    uint64_t cookie,
    const char *path,
    bool is_method_post,
    char **out_mime_type,
    void *ud)
{
    (void) ud;

    *out_mime_type = xstrdup("text/plain");

    Request *req = request_new(cookie);

    const char *normalized_path_cur = strip_leading_slashes(
        path);

    const char *normalized_path_exp = strip_leading_slashes(
        global_options.expected_path);

    if (strcmp(normalized_path_cur, normalized_path_exp) != 0) {
        return request_prepare_error(req, 404, "Not Found");
    }
    if (is_method_post != global_options.expected_method_is_post) {
        return request_prepare_error(req, 405, "Method Not Allowed");
    }
    req->status = 200;
    return req->status;
}

static char *send_prepared_error(Request *req, size_t *out_len)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "%d %s", req->status, req->status_text);
    *out_len = strlen(buf);
    return xstrdup(buf);
}

static char *my_write_body_cb(
    uint64_t cookie,
    const char *body,
    size_t nbody,
    size_t *out_len,
    void *ud)
{
    (void) ud;

    Request *req = request_find_or_die(cookie);
    char *ret;

    int status = req->status;
    if (status != 200) {
        ret = send_prepared_error(req, out_len);
        goto done;
    }

    if (putchar('>') == EOF) {
        goto write_error;
    }
    for (size_t i = 0; i < nbody; ++i) {
        char c = body[i];
        if (c == '\n') {
            c = ' ';
        }
        if (putchar((unsigned char) c) == EOF) {
            goto write_error;
        }
    }
    if (putchar('\n') == EOF) {
        goto write_error;
    }
    if (fflush(stdout) == EOF) {
        goto write_error;
    }

    char *buf = NULL;
    size_t nbuf = 0;
    ssize_t r = getline(&buf, &nbuf, stdin);
    if (r < 0) {
        perror("getline");
        panic("getline() failed");
    } else if (r == 0) {
        panic("got EOF");
    }

    if (buf[r - 1] == '\n') {
        --r;
    }
    *out_len = r;
    ret = buf;

done:
    request_free(req);
    return ret;

write_error:
    panic("cannot write to stdout");
}

static void freeze_or_die(void)
{
    if (global_options.freeze_for < 0) {
        fprintf(stderr, "Existing as requested by --max-requests=...\n");
        exit(0);
    } else if (global_options.freeze_for > 0) {
        fprintf(stderr, "Freezing as requested by --freeze-for=...\n");
        sleep_millis(global_options.freeze_for);
    }
}

static void my_after_req_cb(
    void *ud)
{
    (void) ud;

    static unsigned req_count = 0;
    ++req_count;
    if (
        global_options.max_requests >= 0 &&
        req_count == (unsigned) global_options.max_requests)
    {
        freeze_or_die();
        req_count = 0;
    }
}

static void my_ready_cb(
    void *ud)
{
    (void) ud;

    if (fputs("ready\n", stdout) == EOF) {
        goto fail;
    }
    if (fflush(stdout) == EOF) {
        goto fail;
    }
    return;
fail:
    panic("cannot write to stdout");
}

ATTR_NORETURN
static void print_usage_and_die(const char *s)
{
    fprintf(stderr, "Wrong USAGE: %s\n", s);
    fprintf(stderr, "USAGE: httpserv [--port=PORT] [--max-requests=N] [--freeze-for=MILLIS] METHOD PATH\n");
    exit(2);
}

static bool parse_method_or_die(const char *s)
{
    if (strcmp(s, "GET") == 0) {
        return false;
    }
    if (strcmp(s, "POST") == 0) {
        return true;
    }
    print_usage_and_die("invalid METHOD");
}

int main(int argc, char **argv)
{
    int port = 8080;
    char *pos[2];

    ArgParser_Option options[] = {
        {"--port=", &port},
        {"--max-requests=", &global_options.max_requests},
        {"--freeze-for=", &global_options.freeze_for},
        {0},
    };

    char errbuf[128];
    if (!arg_parser_parse(options, pos, 2, argc, argv, errbuf, sizeof(errbuf))) {
        print_usage_and_die(errbuf);
    }

    if (port < 0 || port > 0xFFFF) {
        print_usage_and_die("invalid PORT");
    }

    global_options.expected_method_is_post = parse_method_or_die(pos[0]);

    global_options.expected_path = xstrdup(pos[1]);

    run_server(
        port,
        my_before_req_cb,
        my_write_body_cb,
        my_after_req_cb,
        my_ready_cb,
        NULL);
}
