#!/bin/sh

set -e

if [ -z "$XXX_AFL_DIR" ]; then
    echo >&2 "You must set the 'XXX_AFL_DIR' environment variable."
    exit 1
fi

# We add '-d' argument for original AFL because otherwise it
# does not terminate within 24 hours.
#
# '-d' means "quick & dirty mode (skips deterministic steps)".

case "$XXX_AFL_IS_PP" in
0)
    extra_opts='-d'
    ;;
1)
    extra_opts=
    ;;
*)
    echo >&2 "You must set 'XXX_AFL_IS_PP' environment variable to either 0 or 1."
    echo >&2 "Set it to 0 if AFL is the original Google's version (not AFL++)."
    echo >&2 "Set it to 1 if AFL is actually AFL++."
    exit 1
esac

# We also set AFL_NO_ARITH=1 because it's a text-based format.
# This potentially speeds up fuzzing.
export AFL_NO_ARITH=1

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

mkdir -p ./findings

export UBSAN_OPTIONS=halt_on_error=1

export AFL_EXIT_WHEN_DONE=1

"$XXX_AFL_DIR"/afl-fuzz $extra_opts -i testcases -o findings -t 5 ./harness @@
