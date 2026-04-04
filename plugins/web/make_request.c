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

#include "make_request.h"
#include <stddef.h>
#include "libls/ls_string.h"
#include "libls/ls_strarr.h"
#include "libls/ls_panic.h"
#include "include/plugin_data_v1.h"
#include "include/sayf_macros.h"
#include "set_error.h"

#define CANNOT_FAIL(expr) \
    do { \
        if ((expr) != CURLE_OK) { \
            LS_PANIC("CANNOT_FAIL() failed"); \
        } \
    } while (0)

static size_t callback_resp(char *buf, size_t char_sz, size_t nbuf, void *ud)
{
    (void) char_sz;

    LS_String *body = ud;
    ls_string_append_b(body, buf, nbuf);

    return nbuf;
}

static size_t callback_header(char *buf, size_t char_sz, size_t nbuf, void *ud)
{
    (void) char_sz;

    LS_StringArray *headers = ud;
    ls_strarr_append(headers, buf, nbuf);

    return nbuf;
}

static const char *get_debug_prefix(curl_infotype type)
{
    switch (type) {
    case CURLINFO_TEXT:
        return "Text";
    case CURLINFO_HEADER_OUT:
        return "Send header";
    case CURLINFO_DATA_OUT:
        return "Send data";
    case CURLINFO_HEADER_IN:
        return "Recv header";
    case CURLINFO_DATA_IN:
        return "Recv data";
    default:
        return NULL;
    }
}

static int callback_debug(CURL *C, curl_infotype type, char *buf, size_t nbuf, void *ud)
{
    (void) C;

    LuastatusPluginData *pd = ud;

    const char *prefix = get_debug_prefix(type);
    if (prefix) {
        enum { MAX_PREVIEW = 1024 * 8 };
        int truncated_len = nbuf < MAX_PREVIEW ? nbuf : MAX_PREVIEW;
        LS_INFOF(pd, "<curl debug> %s: %.*s", prefix, truncated_len, buf);
    }
    return 0;
}

bool make_request(
    LuastatusPluginData *pd,
    int req_flags,
    CURL *C,
    Response *out,
    char **out_errmsg)
{
    *out = (Response) {
        .status = 0,
        .headers = ls_strarr_new(),
        .body = ls_string_new_reserve(1024),
    };

    CANNOT_FAIL(curl_easy_setopt(C, CURLOPT_WRITEFUNCTION, callback_resp));

    CANNOT_FAIL(curl_easy_setopt(C, CURLOPT_WRITEDATA, (void *) &out->body));

    if (req_flags & REQ_FLAG_NEEDS_HEADERS) {
        CANNOT_FAIL(curl_easy_setopt(C, CURLOPT_HEADERFUNCTION, callback_header));
        CANNOT_FAIL(curl_easy_setopt(C, CURLOPT_HEADERDATA, (void *) &out->headers));
    }
    if (req_flags & REQ_FLAG_DEBUG) {
        CANNOT_FAIL(curl_easy_setopt(C, CURLOPT_VERBOSE, 1L));
        CANNOT_FAIL(curl_easy_setopt(C, CURLOPT_DEBUGFUNCTION, callback_debug));
        CANNOT_FAIL(curl_easy_setopt(C, CURLOPT_DEBUGDATA, (void *) pd));
    }

    CURLcode rc = curl_easy_perform(C);
    if (rc != CURLE_OK) {
        set_curl_error(out_errmsg, rc);
        return false;
    }

    CANNOT_FAIL(curl_easy_getinfo(C, CURLINFO_RESPONSE_CODE, &out->status));

    return true;
}
