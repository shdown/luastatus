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

#ifndef httpserv_workmode_h_
#define httpserv_workmode_h_

#include <stddef.h>

void workmode_global_init_fxd(const char *req_file, const char *resp_str);
void workmode_global_init_dyn(const char *req_file, const char *resp_pattern);

struct WorkMode;
typedef struct WorkMode WorkMode;

WorkMode *workmode_new(void);

void workmode_request_chunk(WorkMode *wm, const char *chunk, size_t nchunk);

void workmode_request_finish(WorkMode *wm);

const char *workmode_get_response(WorkMode *wm, int *out_len);

void workmode_destroy(WorkMode *wm);

void workmode_write_to_req_file(const char *s);

void workmode_global_uninit(void);

#endif
