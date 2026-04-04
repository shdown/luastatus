#!/bin/sh

set -e

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

luastatus_root=../../..

"$luastatus_root"/fuzz_utils/gen_testcases/gen_testcases.py \
    ./testcases \
    --a=1:x \
    --b=1:x \
    --mut-substrings="|%2f|%F2|%64|%bA|%2X|%XY" \
    --length=10 \
    --num-files=10 \
    --random-seed=123 \
    --extra-testcase='extra1:%' \
    --extra-testcase='extra2:%%' \
    --extra-testcase='extra3:%%%'
