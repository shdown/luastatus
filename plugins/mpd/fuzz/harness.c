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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "libls/ls_strarr.h"
#include "libsafe/safev.h"
#include "fuzz_utils/fuzz_utils.h"

#include "../safe_haven.h"

#if MODE_CHECK_GREETING

static void do_the_thing(SAFEV v)
{
    char c = is_good_greeting(v);
    fuzz_utils_used(&c, 1);
}

#elif MODE_GET_RESP_TYPE

static void do_the_thing(SAFEV v)
{
    char c = response_type(v);
    fuzz_utils_used(&c, 1);
}

#elif MODE_WRITE_QUOTED

static void do_the_thing(SAFEV v)
{
    char *buf;
    size_t len;

    FILE *f = open_memstream(&buf, &len);
    if (!f) {
        perror("open_memstream");
        abort();
    }

    write_quoted(f, v);

    if (fclose(f) < 0) {
        perror("fclose");
        abort();
    }

    fuzz_utils_used(buf, len);

    free(buf);
}

#elif MODE_APPEND_KV

static void do_the_thing(SAFEV v)
{
    LS_StringArray sa = ls_strarr_new_reserve(1024, 2);

    append_line_to_kv_strarr(&sa, v);

    size_t n = ls_strarr_size(sa);
    for (size_t i = 0; i < n; ++i) {
        size_t ns;
        const char *s = ls_strarr_at(sa, i, &ns);
        fuzz_utils_used(s, ns);
    }

    ls_strarr_destroy(sa);
}

#else
# error "Please define to 1 either: MODE_CHECK_GREETING || MODE_GET_RESP_TYPE || MODE_WRITE_QUOTED || MODE_APPEND_KV."
#endif

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "USAGE: harness_* INPUT_FILE\n");
        return 2;
    }

    int fd_in = open(argv[1], O_RDONLY | O_CLOEXEC);
    if (fd_in < 0) {
        perror(argv[1]);
        abort();
    }

    FuzzInput input = fuzz_input_new_prealloc(1024);
    if (fuzz_input_read(fd_in, &input) < 0) {
        perror("read");
        abort();
    }

    SAFEV v = SAFEV_new_UNSAFE(input.data, input.size);

    do_the_thing(v);

    fuzz_input_free(input);
    close(fd_in);

    return 0;
}
