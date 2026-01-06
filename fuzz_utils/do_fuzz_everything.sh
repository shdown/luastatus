#!/usr/bin/env bash

set -e

shopt -s nullglob

if [[ -z "$XXX_AFL_DIR" ]]; then
    echo >&2 "You must set the 'XXX_AFL_DIR' environment variable."
    exit 1
fi

case "$XXX_AFL_IS_PP" in
0)
    ;;
1)
    ;;
*)
    echo >&2 "You must set 'XXX_AFL_IS_PP' environment variable to either 0 or 1."
    echo >&2 "Set it to 0 if AFL is the original Google's version (not AFL++)."
    echo >&2 "Set it to 1 if AFL is actually AFL++."
    exit 1
    ;;
esac

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

cd ..

fuzz_dirs=(
    plugins/*/fuzz*
    barlibs/*/fuzz*
)

if (( XXX_AFL_IS_PP )); then
    afl_cc="$XXX_AFL_DIR"/afl-cc
else
    afl_cc="$XXX_AFL_DIR"/afl-gcc
fi

for d in "${fuzz_dirs[@]}"; do
    [[ -d "$d" ]] || continue

    "$d"/clear.sh

    CC="$afl_cc" "$d"/build.sh
    if [[ -x "$d"/fuzz_all.sh ]]; then
        "$d"/fuzz_all.sh
    else
        "$d"/fuzz.sh
    fi
done
