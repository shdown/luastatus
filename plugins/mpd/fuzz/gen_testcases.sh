#!/bin/sh

set -e

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

luastatus_root=../../..

common_args_noab='--length=5-20 --random-seed=123'
common_args="--a=1:a --b=1:b $common_args_noab"

# check_greeting
"$luastatus_root"/fuzz_utils/gen_testcases/gen_testcases.py \
    ./testcases_check_greeting \
    $common_args \
    --mut-prefix='OK MPD ' \
    --num-files=10 \
    --extra-testcase='pfx_only:OK MPD '

# get_resp_type: OK
"$luastatus_root"/fuzz_utils/gen_testcases/gen_testcases.py \
    ./testcases_get_resp_type \
    $common_args \
    --mut-prefix='OK' \
    --extra-testcase='pfx_only:OK' \
    --num-files=5 \
    --file-prefix='ok_'

# get_resp_type: ACK
"$luastatus_root"/fuzz_utils/gen_testcases/gen_testcases.py \
    ./testcases_get_resp_type \
    $common_args \
    --mut-prefix='ACK ' \
    --num-files=5 \
    --file-prefix='ack_'

# get_resp_type: other
"$luastatus_root"/fuzz_utils/gen_testcases/gen_testcases.py \
    ./testcases_get_resp_type \
    $common_args \
    --num-files=5 \
    --file-prefix='other_'

# write_quoted
"$luastatus_root"/fuzz_utils/gen_testcases/gen_testcases.py \
    ./testcases_write_quoted \
    $common_args_noab \
    --a=1:'"' \
    --b=1:xyz \
    --a-is-important \
    --num-files=10

# append_kv
"$luastatus_root"/fuzz_utils/gen_testcases/gen_testcases.py \
    ./testcases_append_kv \
    $common_args \
    --mut-substrings='|: |:|' \
    --num-files=10
