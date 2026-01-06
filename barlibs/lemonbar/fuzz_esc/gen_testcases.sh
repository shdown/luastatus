#!/bin/sh

set -e

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

luastatus_root=../../..

"$luastatus_root"/fuzz_utils/gen_testcases/gen_testcases.py \
    ./testcases \
    --a=1:abc \
    --b=1:'%' \
    --length=5-20 \
    --num-files=10 \
    --random-seed=123
