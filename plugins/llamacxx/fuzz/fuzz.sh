#!/bin/sh

set -e

if [ -z "$XXX_AFL_DIR" ]; then
    echo >&2 "You must set the 'XXX_AFL_DIR' environment variable."
    exit 1
fi

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

mkdir -p ./findings

export UBSAN_OPTIONS=halt_on_error=1

export AFL_EXIT_WHEN_DONE=1

"$XXX_AFL_DIR"/afl-fuzz -i testcases -o findings -t 5 ./harness @@
