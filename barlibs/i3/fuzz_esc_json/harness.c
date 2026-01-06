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
#include <unistd.h>
#include <fcntl.h>

#include "libls/ls_string.h"
#include "libsafe/safev.h"
#include "fuzz_utils/fuzz_utils.h"

#include "../escape_json_str.h"

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "USAGE: harness INPUT_FILE\n");
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

    LS_String res = ls_string_new_from_s("escape result = ");
    append_json_escaped_str(&res, SAFEV_new_UNSAFE(input.data, input.size));

    fuzz_utils_used(res.data, res.size);

    fuzz_input_free(input);
    ls_string_free(res);
    close(fd_in);

    return 0;
}
