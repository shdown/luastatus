#!/bin/sh

if [ -z "$CC" ]; then
    echo >&2 "You must set the 'CC' environment variable."
    echo >&2 "Hint: you probably want to set 'CC' to 'some-directory/afl-gcc'."
    exit 1
fi

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

luastatus_root=../../..

$CC -Wall -Wextra -O3 -std=c99 -D_POSIX_C_SOURCE=200809L \
    -I"$luastatus_root" \
    ./harness.c \
    ../escape_json_str.c \
    "$luastatus_root"/libls/string_.c \
    -o harness
