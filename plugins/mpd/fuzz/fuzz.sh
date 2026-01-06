#!/bin/sh

set -e

if [ -z "$XXX_AFL_DIR" ]; then
    echo >&2 "You must set the 'XXX_AFL_DIR' environment variable."
    exit 1
fi

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

case "$1" in
1)
    suffix=check_greeting
    ;;
2)
    suffix=get_resp_type
    ;;
3)
    suffix=write_quoted
    ;;
4)
    suffix=append_kv
    ;;
*)
    printf '%s\n' >&2 "USAGE: $0 {1 | 2 | 3 | 4}"
    exit 2
    ;;
esac

findings_dir=./findings_"$suffix"
testcases_dir=./testcases_"$suffix"
harness_binary=./harness_"$suffix"

mkdir -p "$findings_dir"

export UBSAN_OPTIONS=halt_on_error=1

export AFL_EXIT_WHEN_DONE=1

"$XXX_AFL_DIR"/afl-fuzz -i "$testcases_dir" -o "$findings_dir" -t 5 "$harness_binary" @@
