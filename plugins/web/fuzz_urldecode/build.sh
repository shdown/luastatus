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
    ../mod_urlencode/urldecode.c \
    "$luastatus_root"/libsafe/*.c \
    "$luastatus_root"/libls/ls_panic.c \
    "$luastatus_root"/libls/ls_cstring_utils.c \
    -o harness
