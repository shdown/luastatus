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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "libls/ls_panic.h"
#include "libsafe/safev.h"
#include "libsafe/mut_safev.h"
#include "fuzz_utils/fuzz_utils.h"

#include "../mod_urlencode/urldecode.h"

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

    SAFEV src = SAFEV_new_UNSAFE(input.data, input.size);
    size_t n = urldecode_size(src);
    char *ptr = calloc(n, 1);
    if (!ptr && n) {
        perror("calloc");
        abort();
    }
    MUT_SAFEV dst = MUT_SAFEV_new_UNSAFE(ptr, n);
    char is_ok = urldecode(src, dst);

    fuzz_utils_used(&is_ok, 1);
    fuzz_utils_used(ptr, n);

    free(ptr);
    fuzz_input_free(input);
    close(fd_in);

    return 0;
}
