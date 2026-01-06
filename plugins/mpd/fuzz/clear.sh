#!/bin/sh

set -e

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

rm -rf \
    ./findings_check_greeting \
    ./findings_get_resp_type \
    ./findings_write_quoted \
    ./findings_append_kv
