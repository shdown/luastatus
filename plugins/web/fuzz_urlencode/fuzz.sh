#!/bin/sh

set -e

if [ -z "$XXX_AFL_DIR" ]; then
    echo >&2 "You must set the 'XXX_AFL_DIR' environment variable."
    exit 1
fi

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

case "$1" in
0|1)
    plus_encoding=$1
    ;;
*)
    printf '%s\n' "USAGE: $0 {0 | 1}" >&2
    exit 2
    ;;
esac

mkdir -p ./findings_"$plus_encoding"

export UBSAN_OPTIONS=halt_on_error=1

export AFL_EXIT_WHEN_DONE=1

"$XXX_AFL_DIR"/afl-fuzz $extra_opts -i testcases -o findings_"$plus_encoding" -t 5 ./harness @@ "$plus_encoding"
