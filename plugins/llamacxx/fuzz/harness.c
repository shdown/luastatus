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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "libls/ls_string.h"

#include "../escape_json_str.h"

static LS_String read_fully(int fd)
{
    LS_String res = ls_string_new();
    for (;;) {
        ls_string_ensure_avail(&res, 1);
        ssize_t r = read(fd, res.data + res.size, res.capacity - res.size);
        if (r < 0) {
            perror("read");
            abort();
        } else if (r == 0) {
            break;
        } else {
            res.size += r;
        }
    }
    return res;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "USAGE: harness INPUT_FILE OUTPUT_FILE\n");
        return 2;
    }

    int fd_in = open(argv[1], O_RDONLY | O_CLOEXEC);
    if (fd_in < 0) {
        perror(argv[1]);
        abort();
    }

    int fd_out = open(argv[2], O_WRONLY | O_CLOEXEC | O_APPEND);
    if (fd_out < 0) {
        perror(argv[2]);
        abort();
    }

    LS_String input = read_fully(fd_in);

    LS_String res = ls_string_new_from_s("escape result = ");

    ls_string_append_c(&input, '\0');
    append_json_escaped_s(&res, input.data);

    uint8_t total_sum = 0;
    for (size_t i = 0; i < res.size; ++i) {
        total_sum += (uint8_t) res.data[i];
    }

    if (write(fd_out, &total_sum, 1) < 0) {
        perror("write");
        abort();
    }

    ls_string_free(input);
    ls_string_free(res);

    close(fd_in);
    close(fd_out);

    return 0;
}
