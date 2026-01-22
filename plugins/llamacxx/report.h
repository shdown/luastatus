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

#ifndef report_h_
#define report_h_

#include <stddef.h>
#include <stdbool.h>
#include "libls/ls_compdep.h"
#include "include/plugin_data_v1.h"

typedef enum {
    REPORT_TYPE_CSTR,
    REPORT_TYPE_LSTR,
    REPORT_TYPE_BOOL_TRUE,
    REPORT_TYPE_BOOL_FALSE,
} ReportFieldType;

typedef struct {
    ReportFieldType type;
    const char *k;
    const char *v;
    size_t nv;
} ReportField;

void report_generic(
    LuastatusPluginData *pd,
    LuastatusPluginRunFuncs funcs,
    const ReportField *fields,
    size_t nfields);

LS_INHEADER ReportField report_field_cstr(const char *k, const char *val)
{
    return (ReportField) {
        .type = REPORT_TYPE_CSTR,
        .k = k,
        .v = val,
    };
}

LS_INHEADER ReportField report_field_lstr(const char *k, const char *val, size_t nval)
{
    return (ReportField) {
        .type = REPORT_TYPE_CSTR,
        .k = k,
        .v = val,
        .nv = nval,
    };
}

LS_INHEADER ReportField report_field_bool(const char *k, bool val)
{
    return (ReportField) {
        .type = val ? REPORT_TYPE_BOOL_TRUE : REPORT_TYPE_BOOL_FALSE,
        .k = k,
    };
}

#endif
