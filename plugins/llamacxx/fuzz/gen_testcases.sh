#!/bin/sh

set -e

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

luastatus_root=../../..

"$luastatus_root"/fuzz_utils/gen_testcases/gen_testcases.py \
    ./testcases \
    --a=1:'\"/' \
    --a-range=h:1:0-31 \
    --b=1:abc \
    --b-range=h:1:127-255 \
    --a-is-important \
    --length=5-20 \
    --num-files=10 \
    --random-seed=123
