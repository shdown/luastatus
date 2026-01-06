#!/bin/sh

set -e

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

luastatus_root=../../..

nl=$(printf '\nx')
nl=${nl%x}

# 'fps' means 'final percent sign'
"$luastatus_root"/fuzz_utils/gen_testcases/gen_testcases.py \
    ./testcases \
    --file-prefix='X_' \
    --a=1:x \
    --b=1:x \
    --mut-substrings="|${nl}|}|%{A|%{x|%x|%%" \
    --length=10 \
    --num-files=6 \
    --random-seed=123 \
    --extra-testcase='fps:x%'

"$luastatus_root"/fuzz_utils/gen_testcases/gen_testcases.py \
    ./testcases \
    --file-prefix='Y_' \
    --a=1:x \
    --b=1:x \
    --mut-prefix='%{A' \
    --mut-substrings='|:' \
    --length=10 \
    --num-files=4 \
    --random-seed=123

"$luastatus_root"/fuzz_utils/gen_testcases/gen_testcases.py \
    ./testcases \
    --file-prefix='Z_' \
    --a=1:x \
    --b=1:x \
    --mut-prefix='x%{A' \
    --mut-substrings='|:' \
    --length=10 \
    --num-files=5 \
    --random-seed=123
