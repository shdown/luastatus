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

#include "requester.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <curl/curl.h>
#include <yajl/yajl_tree.h>

#include "libls/ls_string.h"
#include "libls/ls_alloc_utils.h"
#include "libls/ls_xallocf.h"
#include "libsafe/safev.h"

#include "include/sayf_macros.h"

#include "escape_json_str.h"
#include "external_context.h"
#include "my_error.h"
#include "priv.h"

enum { PREVIEW_MAX = 1024 };

struct Requester {
    CURL *curl;
    struct curl_slist *headers;
    char *url;

    uint32_t actual_max_response_bytes;
    const PrivCnxSettings *settings;
    ExternalContext ectx;
};

bool requester_global_init(MyError *out_err)
{
    CURLcode rc = curl_global_init(CURL_GLOBAL_ALL);
    if (rc != CURLE_OK) {
        my_error_printf(
            out_err, "curl_global_init(CURL_GLOBAL_ALL) failed with code %d",
            (int) rc);
        return false;
    }
    return true;
}

Requester *requester_new(
    const PrivCnxSettings *settings,
    ExternalContext ectx,
    MyError *out_err)
{
    CURL *curl = NULL;
    struct curl_slist *headers = NULL;

    curl = curl_easy_init();
    if (!curl) {
        my_error_printf(out_err, "curl_easy_init() failed");
        goto fail;
    }

    headers = curl_slist_append(NULL, "Content-Type: application/json");
    if (!headers) {
        my_error_printf(out_err, "curl_slist_append() failed");
        goto fail;
    }

    const char *proto = settings->use_ssl ? "https" : "http";

    enum { RESPONSE_SIZE_HARD_LIMIT = 0x7fffffff - 1024 };
    uint32_t actual_max_response_bytes = settings->max_response_bytes;
    if (actual_max_response_bytes > RESPONSE_SIZE_HARD_LIMIT) {
        actual_max_response_bytes = RESPONSE_SIZE_HARD_LIMIT;
    }
    Requester *R = LS_XNEW(Requester, 1);
    *R = (Requester) {
        .curl = curl,
        .headers = headers,
        .url = ls_xallocf("%s://%s:%d/completion", proto, settings->hostname, settings->port),

        .actual_max_response_bytes = actual_max_response_bytes,
        .settings = settings,
        .ectx = ectx,
    };
    return R;

fail:
    curl_slist_free_all(headers);
    if (curl) {
        curl_easy_cleanup(curl);
    }
    return NULL;
}

static inline bool valid_str(const char *s)
{
    return s && s[0] != '\0';
}

static LS_String make_req_body(Requester *R, const char *extra_params_json, SAFEV prompt)
{
    LS_String r = ls_string_new_reserve(1024);

    ls_string_append_s(&r, "{\"prompt\":");
    append_json_escaped_str(&r, prompt);

    ls_string_append_s(&r, ",\"response_fields\":[\"content\"]");

    ls_string_append_s(&r, ",\"cache_prompt\":");
    ls_string_append_s(&r, R->settings->cache_prompt ? "true" : "false");

    if (valid_str(extra_params_json)) {
        ls_string_append_c(&r, ',');
        ls_string_append_s(&r, extra_params_json);
    }

    ls_string_append_c(&r, '}');

    return r;
}

static size_t callback_save_resp(char *buf, size_t char_sz, size_t nbuf, void *ud)
{
    (void) char_sz;

    LS_String *resp_body = ud;
    ls_string_append_b(resp_body, buf, nbuf);

    return nbuf;
}

static bool parse_response(const char *resp, LS_String *out, MyError *out_err)
{
    bool ret = false;
    yajl_val Y_tree = NULL;

    char err_descr[256];
    Y_tree = yajl_tree_parse(resp, err_descr, sizeof(err_descr));
    if (!Y_tree) {
        my_error_set_meta(out_err, 'J', -1);
        my_error_printf(out_err, "JSON parsing error: %s", err_descr);
        goto done;
    }

    static const char *json_path[] = {"content", NULL};
    yajl_val Y_content = yajl_tree_get(Y_tree, json_path, yajl_t_string);
    if (!Y_content) {
        my_error_set_meta(out_err, 'J', -1);
        my_error_printf(out_err, "response does not contain a string 'content' field");
        goto done;
    }

    const char *s = YAJL_GET_STRING(Y_content);
    if (!s) {
        my_error_set_meta(out_err, 'J', -1);
        my_error_printf(out_err, "YAJL_GET_STRING() returned NULL on supposedly string yajl_val");
        goto done;
    }

    ls_string_assign_s(out, s);
    ret = true;

done:
    if (Y_tree) {
        yajl_tree_free(Y_tree);
    }
    return ret;
}

static const char *info_type_to_str(curl_infotype type)
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
    case CURLINFO_SSL_DATA_OUT:
    case CURLINFO_SSL_DATA_IN:
    default:
        return NULL;
    }
}

static int callback_debug(CURL *easy_handle, curl_infotype type, char *data, size_t len, void *ud)
{
    (void) easy_handle;

    Requester *R = ud;
    ExternalContext ectx = R->ectx;

    const char *type_str = info_type_to_str(type);
    if (type_str) {
        if (len > PREVIEW_MAX) {
            len = PREVIEW_MAX;
        }
        LS_INFOF(ectx, "CURL debug: %s: %.*s", type_str, (int) len, data);
    }

    return 0;
}

static void do_log_response_on_error(Requester *R, LS_String resp_body)
{
    size_t len = resp_body.size;
    if (len > PREVIEW_MAX) {
        len = PREVIEW_MAX;
    }
    LS_INFOF(
        R->ectx, "Response body (log_response_on_error enabled): %.*s",
        (int) len, resp_body.data);
}

bool requester_make_request(
    Requester *R,
    const char *extra_params_json,
    SAFEV prompt,
    LS_String *out,
    MyError *out_err)
{
    bool ret = false;
    const char *bad_curl_where = NULL;
    CURLcode bad_curl_rc;
    LS_String resp_body = ls_string_new_reserve(1024);
    LS_String req_body = make_req_body(R, extra_params_json, prompt);

    enum { LIMIT = 8000000 };
    if (req_body.size >= LIMIT) {
        my_error_set_meta(out_err, 'L', 0);
        my_error_printf(
            out_err, "request body would be too big (must be < %d bytes)",
            (int) LIMIT);
        goto done;
    }

    CURL *curl = R->curl;

#define BAD_CURL_CHECK(Where_, Expr_) \
    do { \
        if ((bad_curl_rc = (Expr_)) != CURLE_OK) { \
            bad_curl_where = (Where_); \
            goto done; \
        } \
    } while (0)

#define BAD_CURL_CHECK_SETOPT(Opt_, Val_) \
    BAD_CURL_CHECK( \
        "curl_easy_setopt: " #Opt_, \
        curl_easy_setopt(curl, (Opt_), (Val_)))

    curl_easy_reset(curl);

    BAD_CURL_CHECK_SETOPT(CURLOPT_URL, R->url);

    BAD_CURL_CHECK_SETOPT(CURLOPT_HTTPHEADER, R->headers);

    BAD_CURL_CHECK_SETOPT(CURLOPT_WRITEFUNCTION, callback_save_resp);
    BAD_CURL_CHECK_SETOPT(CURLOPT_WRITEDATA, (void *) &resp_body);

    if (R->settings->log_all_traffic) {
        BAD_CURL_CHECK_SETOPT(CURLOPT_VERBOSE, 1L);
        BAD_CURL_CHECK_SETOPT(CURLOPT_DEBUGFUNCTION, callback_debug);
        BAD_CURL_CHECK_SETOPT(CURLOPT_DEBUGDATA, (void *) R);
    }

    if (R->settings->req_timeout > 0) {
        BAD_CURL_CHECK_SETOPT(CURLOPT_TIMEOUT, (long) R->settings->req_timeout);
    }

    BAD_CURL_CHECK_SETOPT(CURLOPT_POST, 1L);
    BAD_CURL_CHECK_SETOPT(CURLOPT_POSTFIELDS, (char *) req_body.data);
    BAD_CURL_CHECK_SETOPT(CURLOPT_POSTFIELDSIZE, (long) req_body.size);

    if (valid_str(R->settings->custom_iface)) {
        BAD_CURL_CHECK_SETOPT(CURLOPT_INTERFACE, (char *) R->settings->custom_iface);
    }

    BAD_CURL_CHECK_SETOPT(CURLOPT_MAXFILESIZE, (long) R->actual_max_response_bytes);

    BAD_CURL_CHECK("curl_easy_perform", curl_easy_perform(curl));

    long resp_code;
    BAD_CURL_CHECK("curl_easy_getinfo", curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &resp_code));

    if (!(resp_code >= 200 && resp_code <= 299)) {
        my_error_set_meta(out_err, 'H', (int) resp_code);
        if (resp_code) {
            my_error_printf(out_err, "HTTP status %ld", resp_code);
            if (R->settings->log_response_on_error) {
                do_log_response_on_error(R, resp_body);
            }
        } else {
            my_error_printf(out_err, "No response received");
        }
        goto done;
    }

    if (resp_body.size > R->actual_max_response_bytes) {
        my_error_set_meta(out_err, 'L', 1);
        my_error_printf(out_err, "Response is too big");
        goto done;
    }

    ls_string_append_c(&resp_body, '\0');
    if (!parse_response(resp_body.data, out, out_err)) {
        goto done;
    }

    ret = true;

done:
    if (bad_curl_where) {
        const char *descr = curl_easy_strerror(bad_curl_rc);
        if (!descr) {
            descr = "(NULL)";
        }
        my_error_set_meta(out_err, 'C', (int) bad_curl_rc);
        my_error_printf(
            out_err, "CURL error: %s: %s",
            bad_curl_where,
            descr);
    }
    ls_string_free(resp_body);
    ls_string_free(req_body);

    return ret;

#undef BAD_CURL_CHECK
#undef BAD_CURL_CHECK_SETOPT
}

void requester_destroy(Requester *R)
{
    curl_easy_cleanup(R->curl);
    curl_slist_free_all(R->headers);
    free(R->url);
}
