#!/bin/sh

if [ -z "$CC" ]; then
    echo >&2 "You must set the 'CC' environment variable."
    echo >&2 "Hint: you probably want to set 'CC' to 'some-directory/afl-gcc'."
    exit 1
fi

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

luastatus_root=../../..

$CC -Wall -Wextra -O3 -fsanitize=undefined -std=c99 -D_POSIX_C_SOURCE=200809L \
    -I"$luastatus_root" \
    ./harness.c \
    ../pango_escape.c \
    "$luastatus_root"/libls/ls_string.c \
    "$luastatus_root"/libls/ls_alloc_utils.c \
    "$luastatus_root"/libls/ls_panic.c \
    "$luastatus_root"/libls/ls_assert.c \
    "$luastatus_root"/libls/ls_cstring_utils.c \
    "$luastatus_root"/libsafe/*.c \
    -o harness
