#!/bin/sh

set -e

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

luastatus_root=../../..

nl=$(printf '\nx')
nl=${nl%x}

"$luastatus_root"/fuzz_utils/gen_testcases/gen_testcases.py \
    ./testcases \
    --a=1:"$nl" \
    --b=1:xyz \
    --a-is-important \
    --length=5-20 \
    --num-files=10 \
    --random-seed=123
