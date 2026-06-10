/*
 * Copyright (C) 2015-2026  luastatus developers
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
#include <stdbool.h>
#include <string.h>
#include <lua.h>

#include "include/plugin_v1.h"
#include "include/sayf_macros.h"

#include "libmoonvisit/moonvisit.h"

#include "libls/ls_alloc_utils.h"
#include "libls/ls_panic.h"
#include "libls/ls_getenv_r.h"

#include "cmd_output.h"
#include "ipc_client.h"
#include "compat_include_cjson.h"

typedef struct {
    char *socket_path;
    bool report_all_names;

    bool debug;
    bool no_env_var;
} Priv;

static void destroy(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;

    free(p->socket_path);

    free(p);
}

static int init(LuastatusPluginData *pd, lua_State *L)
{
    Priv *p = pd->priv = LS_XNEW(Priv, 1);
    *p = (Priv) {
        .socket_path = NULL,
        .report_all_names = false,
        .debug = false,
        .no_env_var = false,
    };

    char errbuf[256];
    MoonVisit mv = {.L = L, .errbuf = errbuf, .nerrbuf = sizeof(errbuf)};

    // Parse socket_path
    if (moon_visit_str(&mv, -1, "socket_path", &p->socket_path, NULL, true) < 0) {
        goto mverror;
    }

    // Parse report_all_names
    if (moon_visit_bool(&mv, -1, "report_all_names", &p->report_all_names, true) < 0) {
        goto mverror;
    }

    // Parse debug
    if (moon_visit_bool(&mv, -1, "debug", &p->debug, true) < 0) {
        goto mverror;
    }

    // Parse no_env_var
    if (moon_visit_bool(&mv, -1, "no_env_var", &p->no_env_var, true) < 0) {
        goto mverror;
    }

    return LUASTATUS_OK;

mverror:
    LS_FATALF(pd, "%s", errbuf);
//error:
    destroy(pd);
    return LUASTATUS_ERR;
}

static char *get_socket_path(LuastatusPluginData *pd)
{
    Priv *p = pd->priv;

    if (p->socket_path) {
        return ls_xstrdup(p->socket_path);
    }

    if (!p->no_env_var) {
        const char *envvar = ls_getenv_r("SWAYSOCK");
        if (envvar && envvar[0]) {
            return ls_xstrdup(envvar);
        }
    }

    return cmd_output(pd, "sway --get-socketpath");
}

typedef struct {
    LuastatusPluginData *pd;
    LuastatusPluginRunFuncs funcs;
} Context;

static bool push_all_names(lua_State *L, cJSON *j)
{
    cJSON *j_names = cJSON_GetObjectItemCaseSensitive(j, "xkb_layout_names");
    if (!j_names) {
        return false;
    }
    if (!cJSON_IsArray(j_names)) {
        return false;
    }

    int n = cJSON_GetArraySize(j_names);
    lua_createtable(L, n, 0); // L: array
    unsigned i = 1;
    for (cJSON *item = j_names->child; item; item = item->next) {

        const char *name = cJSON_GetStringValue(item);
        if (!name) {
            name = "";
        }

        lua_pushstring(L, name);
        // L: array str
        lua_rawseti(L, -2, i); // L: array
        ++i;
    }

    return true;
}

static const char *fetch_str(cJSON *j_dict, const char *key, const char *alt)
{
    cJSON *j_val = cJSON_GetObjectItemCaseSensitive(j_dict, key);
    if (!j_val) {
        return alt;
    }
    const char *val = cJSON_GetStringValue(j_val);
    if (!val) {
        return alt;
    }
    return val;
}

static int fetch_int(cJSON *j_dict, const char *key, int alt)
{
    cJSON *j_val = cJSON_GetObjectItemCaseSensitive(j_dict, key);
    if (!j_val) {
        return alt;
    }
    if (!cJSON_IsNumber(j_val)) {
        return alt;
    }
    return j_val->valueint;
}

static int push_input_info(lua_State *L, cJSON *j, bool report_all_names)
{
    if (!cJSON_IsObject(j)) {
        return -1;
    }

    const char *type = fetch_str(j, "type", "");
    if (strcmp(type, "keyboard") != 0) {
        return 0;
    }

    const char *ident = fetch_str(j, "identifier", "");
    const char *active_name = fetch_str(j, "xkb_active_layout_name", "");
    int active_idx = fetch_int(j, "xkb_active_layout_index", -1);

    int prealloc = report_all_names ? 4 : 3;
    lua_createtable(L, 0, prealloc); // L: ? table

    lua_pushstring(L, ident); // L: ? table str
    lua_setfield(L, -2, "ident"); // L: ? table

    lua_pushstring(L, active_name); // L: ? table str
    lua_setfield(L, -2, "active_name"); // L: ? table

    lua_pushinteger(L, active_idx); // L: ? table int
    lua_setfield(L, -2, "active_idx"); // L: ? table

    if (report_all_names) {
        if (push_all_names(L, j)) {
            // L: ? table array
            lua_setfield(L, -2, "all_names"); // L: ? table
        }
    }

    return 1;
}

static bool my_ipc_client_cb(cJSON *j, void *ud)
{
    Context *ctx = ud;
    Priv *p = ctx->pd->priv;
    bool ok = true;

    LS_ASSERT(cJSON_IsArray(j));

    lua_State *L = ctx->funcs.call_begin(ctx->pd->userdata);

    lua_newtable(L); // L: table

    unsigned i = 1;
    for (cJSON *item = j->child; item; item = item->next) {
        int rc = push_input_info(L, item, p->report_all_names);
        if (rc < 0) {
            LS_ERRF(ctx->pd, "reply has unexpected JSON shape");
            ok = false;
        }

        if (rc > 0) {
            // L: table value
            lua_rawseti(L, -2, i); // L: table
            ++i;
        }
    }

    ctx->funcs.call_end(ctx->pd->userdata);

    return ok;
}

static void run(LuastatusPluginData *pd, LuastatusPluginRunFuncs funcs)
{
    Priv *p = pd->priv;

    char *socket_path = NULL;
    IpcClient *C = NULL;

    socket_path = get_socket_path(pd);
    if (!socket_path) {
        LS_FATALF(pd, "cannot detect socket path");
        goto error;
    }

    Context ctx = {
        .pd = pd,
        .funcs = funcs,
    };
    C = ipc_client_create(pd, socket_path, my_ipc_client_cb, &ctx, p->debug);
    if (!C) {
        goto error;
    }

    ipc_client_interact(C);

error:
    free(socket_path);
    if (C) {
        ipc_client_destroy(C);
    }
}

LuastatusPluginIface luastatus_plugin_iface_v1 = {
    .init = init,
    .run = run,
    .destroy = destroy,
};
