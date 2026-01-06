#!/bin/sh

set -e

if [ -z "$CC" ]; then
    echo >&2 "You must set the 'CC' environment variable."
    echo >&2 "Hint: you probably want to set 'CC' to 'some-directory/afl-gcc'."
    exit 1
fi

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

luastatus_root=../../..

do_build_variant() {
    local extra_opts=$1
    local output=$2

    printf '%s\n' >&2 "Building with $extra_opts -> $output"

    $CC -Wall -Wextra -O3 -fsanitize=undefined -std=c99 -D_POSIX_C_SOURCE=200809L \
        $extra_opts \
        -I"$luastatus_root" \
        ./harness.c \
        ../safe_haven.c \
        "$luastatus_root"/libls/ls_string.c \
        "$luastatus_root"/libls/ls_alloc_utils.c \
        "$luastatus_root"/libls/ls_panic.c \
        "$luastatus_root"/libls/ls_assert.c \
        "$luastatus_root"/libls/ls_cstring_utils.c \
        "$luastatus_root"/libsafe/*.c \
        -o "$output"
}

do_build_variant -DMODE_CHECK_GREETING=1 harness_check_greeting
do_build_variant -DMODE_GET_RESP_TYPE=1 harness_get_resp_type
do_build_variant -DMODE_WRITE_QUOTED=1 harness_write_quoted
do_build_variant -DMODE_APPEND_KV=1 harness_append_kv
