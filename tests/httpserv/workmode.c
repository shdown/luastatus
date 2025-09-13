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

#include "workmode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include "fatal_error.h"

enum { DIGITS_MAX = 256 };

static bool full_write_bool(int fd, const char *data, size_t ndata)
{
    size_t nwritten = 0;
    while (nwritten < ndata) {
        ssize_t w = write(fd, data + nwritten, ndata - nwritten);
        if (w < 0) {
            return false;
        }
        nwritten += w;
    }
    return true;
}

typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} Buf;

static void buf_ensure(Buf *b, size_t n)
{
    size_t new_c = b->capacity;
    if (!new_c) {
        new_c = 1;
    }
    while (new_c - b->size < n) {
        if (new_c > SIZE_MAX / 2) {
            OUT_OF_MEMORY();
        }
        new_c *= 2;
    }
    if (!(b->data = realloc(b->data, new_c))) {
        OUT_OF_MEMORY();
    }
    b->capacity = new_c;
}

static inline Buf buf_new(void)
{
    return (Buf) {0};
}

static inline void buf_append(Buf *b, const char *data, size_t ndata)
{
    buf_ensure(b, ndata);
    if (ndata) {
        memcpy(b->data + b->size, data, ndata);
    }
    b->size += ndata;
}

static inline void buf_free(Buf *b)
{
    free(b->data);
}

struct WorkMode {
    Buf buf;
};

enum {
    MODE_FXD = 0,
    MODE_DYN = 1,
};
static int mode = -1;
static char *fxd_resp_str;
static char *dyn_resp_pattern;
static char *req_file;
static int req_file_fd;

static inline void open_req_file_fd_or_die(const char *path)
{
    fprintf(stderr, "httpserv: opening req-file '%s'...\n", path);

    req_file_fd = open(path, O_WRONLY | O_CLOEXEC);
    if (req_file_fd < 0) {
        PERROR2_AND_DIE("open", path);
    }

    fprintf(stderr, "httpserv: done opening req-file\n");
}

static inline void check_len_or_die(const char *s, const char *what)
{
    if (strlen(s) > INT_MAX - DIGITS_MAX - 100) {
        FATAL("%s is too long.\n", what);
    }
}

static inline char *strdup_or_die(const char *s)
{
    size_t ns = strlen(s);
    char *r = malloc(ns + 1);
    if (!r) {
        OUT_OF_MEMORY();
    }
    memcpy(r, s, ns + 1);
    return r;
}

void workmode_global_init_fxd(const char *req_file_, const char *resp_str)
{
    assert(mode == -1);

    mode = MODE_FXD;
    fxd_resp_str = strdup_or_die(resp_str);
    req_file = strdup_or_die(req_file_);

    check_len_or_die(fxd_resp_str, "resp-string");
    open_req_file_fd_or_die(req_file_);
}

void workmode_global_init_dyn(const char *req_file_, const char *resp_pattern)
{
    assert(mode == -1);

    mode = MODE_DYN;
    dyn_resp_pattern = strdup_or_die(resp_pattern);
    req_file = strdup_or_die(req_file_);

    check_len_or_die(dyn_resp_pattern, "resp-pattern");
    open_req_file_fd_or_die(req_file_);
}

WorkMode *workmode_new(void)
{
    assert(mode != -1);

    WorkMode *wm = malloc(sizeof(WorkMode));
    if (!wm) {
        OUT_OF_MEMORY();
    }
    wm->buf = buf_new();
    return wm;
}

void workmode_request_chunk(WorkMode *wm, const char *chunk, size_t nchunk)
{
    assert(mode != -1);

    buf_append(&wm->buf, chunk, nchunk);
}

void workmode_request_finish(WorkMode *wm)
{
    assert(mode != -1);

    char *p = wm->buf.data;
    size_t n = wm->buf.size;
    for (size_t i = 0; i < n; ++i) {
        if (p[i] == '\n') {
            p[i] = '\t';
        }
    }

    bool ok = full_write_bool(req_file_fd, p, n);
    if (ok) {
        ok = full_write_bool(req_file_fd, "\n", 1);
    }

    if (!ok) {
        PERROR2_AND_DIE("write", req_file);
    }
}

static inline bool is_ascii_digit(char c)
{
    return '0' <= c && c <= '9';
}

static const char *first_group_of_digits(const char *src, size_t len, size_t *out_len)
{
    size_t i = 0;
    while (i < len && !is_ascii_digit(src[i])) {
        ++i;
    }
    size_t begin_i = i;
    while (i < len && is_ascii_digit(src[i])) {
        ++i;
    }
    *out_len = i - begin_i;
    return src + begin_i;
}

const char *workmode_get_response(WorkMode *wm, int *out_len)
{
    assert(mode != -1);

    if (mode == MODE_FXD) {
        *out_len = strlen(fxd_resp_str);
        return fxd_resp_str;

    } else if (mode == MODE_DYN) {
        size_t digits_len;
        const char *digits = first_group_of_digits(
            wm->buf.data,
            wm->buf.size,
            &digits_len);

        if (!digits_len || digits_len > DIGITS_MAX) {
            *out_len = 0;
            return NULL;
        }

        const char *at = strchr(dyn_resp_pattern, '@');
        assert(at);

        Buf buf2 = buf_new();
        buf_append(&buf2, dyn_resp_pattern, at - dyn_resp_pattern);
        buf_append(&buf2, digits, digits_len);
        buf_append(&buf2, at + 1, strlen(at + 1));

        buf_free(&wm->buf);
        wm->buf = buf2;

        *out_len = buf2.size;
        return buf2.data;

    } else {
        assert(0);
        return NULL;
    }
}

void workmode_destroy(WorkMode *wm)
{
    assert(mode != -1);

    buf_free(&wm->buf);
    free(wm);
}

void workmode_write_to_req_file(const char *s)
{
    assert(mode != -1);

    if (!full_write_bool(req_file_fd, s, strlen(s))) {
        PERROR2_AND_DIE("write", req_file);
    }
}

void workmode_global_uninit(void)
{
    assert(mode != -1);

    free(fxd_resp_str);
    free(dyn_resp_pattern);
    free(req_file);
    close(req_file_fd);
}
