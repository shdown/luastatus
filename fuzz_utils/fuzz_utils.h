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

#ifndef luastatus_fuzz_utils_h_
#define luastatus_fuzz_utils_h_

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

// At least for fuzzing, please use a reasonably modern GNU C-compatible compiler.

#define FUZZ_UTILS_INHEADER \
    static inline __attribute__((unused))

#define FUZZ_UTILS_ATTR_WARN_UNUSED \
    __attribute__((warn_unused_result))

#define FUZZ_UTILS_OOM() \
    do { \
        fputs("fuzz_utils: out of memory.\n", stderr); \
        abort(); \
    } while (0)

typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} FuzzInput;

FUZZ_UTILS_INHEADER FuzzInput fuzz_input_new_prealloc(size_t prealloc)
{
    if (!prealloc) {
        return (FuzzInput) {0};
    }
    char *data = malloc(prealloc);
    if (!data) {
        FUZZ_UTILS_OOM();
    }
    return (FuzzInput) {
        .data = data,
        .size = 0,
        .capacity = prealloc,
    };
}

FUZZ_UTILS_INHEADER void fuzz_input__ensure1(FuzzInput *x)
{
    if (x->size != x->capacity) {
        return;
    }

    if (x->capacity) {
        if (x->capacity > SIZE_MAX / 2) {
            goto oom;
        }
        x->capacity *= 2;
    } else {
        x->capacity = 1;
    }
    if (!(x->data = realloc(x->data, x->capacity))) {
        goto oom;
    }
    return;

oom:
    FUZZ_UTILS_OOM();
}

FUZZ_UTILS_INHEADER FUZZ_UTILS_ATTR_WARN_UNUSED
int fuzz_input_read(int fd, FuzzInput *dst)
{
    for (;;) {
        fuzz_input__ensure1(dst);
        ssize_t r = read(fd, dst->data + dst->size, dst->capacity - dst->size);
        if (r < 0) {
            return -1;
        } else if (r == 0) {
            return 0;
        }
        dst->size += r;
    }
}

FUZZ_UTILS_INHEADER void fuzz_input_free(FuzzInput x)
{
    free(x.data);
}

FUZZ_UTILS_INHEADER void fuzz_utils_used(const char *ptr, size_t len)
{
    // This is a compiler barrier: there's a contract between the compiler and
    // the user that the compiler doesn't try to guess what inline assembly does,
    // even if its body is empty.
    //
    // So after this, the compiler conservately thinks the span is used, because
    // the inline assembly might have dereferenced it and done something that has
    // side effects (e.g. made a system call "write(1, ptr, len)").
    __asm__ __volatile__ (
        ""
        :
        : "r"(ptr), "r"(len)
        : "memory"
    );
}

#endif
