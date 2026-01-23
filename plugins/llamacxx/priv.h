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

#ifndef priv_h_
#define priv_h_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <lauxlib.h>
#include "libls/ls_io_utils.h"
#include "libls/ls_compdep.h"
#include "libls/ls_evloop_lfuncs.h"
#include "dss_list.h"
#include "pushed_str.h"
#include "map_ref.h"

enum {
    DEFAULT_TMO_REQ = 7,
    DEFAULT_TMO_UPD = 3,
};

enum { DEFAULT_PORT = 8080 };

typedef struct {
    char *hostname;
    int port;
    char *custom_iface;
    bool use_ssl;
    bool log_all_traffic;
    bool log_response_on_error;
    bool cache_prompt;
    uint32_t max_response_bytes;
    int req_timeout;
} PrivCnxSettings;

typedef struct {
    PrivCnxSettings cnx_settings;

    bool greet;
    double upd_timeout;
    bool tell_about_timeout;
    bool report_mu;
    char *extra_params_json;

    int lref_prompt_func;

    DSS_List *dss_list;

    PushedStr pushed_extra_params_json;

    int self_pipe[2];

    LS_PushedTimeout pushed_tmo;

    MapRef map_ref;
} Priv;

LS_INHEADER PrivCnxSettings priv_cnx_settings_new(void)
{
    return (PrivCnxSettings) {
        .hostname = NULL,
        .port = DEFAULT_PORT,
        .custom_iface = NULL,
        .use_ssl = false,
        .log_all_traffic = false,
        .log_response_on_error = false,
        .cache_prompt = true,
        .max_response_bytes = UINT32_MAX,
        .req_timeout = DEFAULT_TMO_REQ,
    };
}

LS_INHEADER void priv_init(Priv *p)
{
    *p = (Priv) {
        .cnx_settings = priv_cnx_settings_new(),
        .greet = false,
        .upd_timeout = DEFAULT_TMO_UPD,
        .tell_about_timeout = false,
        .report_mu = false,
        .extra_params_json = NULL,
        .lref_prompt_func = LUA_REFNIL,
        .dss_list = DSS_list_new(),
        .pushed_extra_params_json = {0},
        .self_pipe = {-1, -1},
        .pushed_tmo = {0},
        .map_ref = map_ref_new_empty(),
    };
    ls_pushed_timeout_init(&p->pushed_tmo);
    pushed_str_new(&p->pushed_extra_params_json);
}

LS_INHEADER void priv_cnx_settings_destroy(PrivCnxSettings *x)
{
    free(x->hostname);
    free(x->custom_iface);
}

// "inner" means this function doesn't call 'free(p)'.
LS_INHEADER void priv_destroy_inner(Priv *p)
{
    priv_cnx_settings_destroy(&p->cnx_settings);

    free(p->extra_params_json);

    DSS_list_destroy(p->dss_list);

    ls_close(p->self_pipe[0]);
    ls_close(p->self_pipe[1]);

    ls_pushed_timeout_destroy(&p->pushed_tmo);

    pushed_str_destroy(&p->pushed_extra_params_json);

    map_ref_destroy(p->map_ref);
}

#endif
