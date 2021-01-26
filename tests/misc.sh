#!/usr/bin/env bash

opwd=$PWD
cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")" || exit $?

source ./utils.lib.bash || exit $?

if (( $# != 1 )); then
    echo >&2 "USAGE: $0 <build root>"
    exit 2
fi
BUILD_DIR=$(resolve_relative "$1" "$opwd") || exit $?
HANG_TIMEOUT=${TIMEOUT:-7}
PREFIX=( valgrind --error-exitcode=42 )
LUASTATUS=( "$BUILD_DIR"/luastatus/luastatus ${DEBUG:+-l trace} )
LUASTATUS_PID=

if [[ "$(uname -s)" == Linux ]]; then
    is_process_sleeping() {
        local state
        state=$(awk '$1 == "State:" { print $2 }' /proc/"$1"/status) || return 3
        if [[ $state == S ]]; then
            return 0
        else
            return 1
        fi
    }
else
    is_process_sleeping() {
        true
    }
fi

fail() {
    printf >&2 '%s\n' '=== FAILED ===' "$@"
    exit 1
}

assert_exits_with_code() {
    echo >&2 "===> assert_exits_with_code $*"

    local c=$1
    shift
    "${PREFIX[@]}" "${LUASTATUS[@]}" "$@"
    local got_c=$?
    if (( c != got_c )); then
        fail "Expected exit code $c, found $got_c"
    fi
}

assert_succeeds() {
    assert_exits_with_code 0 "$@"
}

assert_fails() {
    assert_exits_with_code 1 "$@"
}

assert_hangs() {
    echo >&2 "===> assert_hangs $*"

    "${PREFIX[@]}" "${LUASTATUS[@]}" "$@" &
    LUASTATUS_PID=$!

    sleep "$HANG_TIMEOUT" || exit $?

    is_process_sleeping "$LUASTATUS_PID" \
        || fail "The process (PID $LUASTATUS_PID) does not appear to be sleeping."

    echo >&2 "Killing and waiting for termination..."
    kill "$LUASTATUS_PID" || fail "“kill $LUASTATUS_PID” failed"
    wait "$LUASTATUS_PID" || true
    LUASTATUS_PID=

    echo >&2 "done"
}

assert_works() {
    assert_hangs "$@"
    assert_succeeds -e "$@"
}

assert_works_1W() {
    local tail=${@:$#}
    local init=("${@:1:$#-1}")
    assert_hangs "${init[@]}" <(printf '%s\n' "$tail")
    assert_succeeds -e "${init[@]}" <(printf '%s\n' "$tail")
}

trap '
    if [[ -n $LUASTATUS_PID ]]; then
        echo >&2 "Killing luastatus (PID $LUASTATUS_PID)."
        kill $LUASTATUS_PID || true
    fi
' EXIT

assert_fails
assert_fails /dev/null
assert_fails /dev/nil
assert_fails -e
assert_fails -TheseFlagsDoNotExist
assert_fails -l ''
assert_fails -l nosuchloglevel
assert_fails -l '•÷¢˝Q⅓'
assert_fails -l
assert_fails -l -l
assert_fails -ы
assert_fails -v
assert_fails -b

assert_fails -b '§n”™°£'
assert_fails -b '/'
assert_fails -b 'nosuchbarlibforsure'

B=( -b "$BUILD_DIR"/tests/barlib-mock.so )
P="$BUILD_DIR"/tests/plugin-mock.so

assert_succeeds "${B[@]}" -eeeeeeee -e

assert_works "${B[@]}"
assert_works "${B[@]}" .
assert_works "${B[@]}" . . . . .
assert_works "${B[@]}" /dev/null
assert_works "${B[@]}" /dev/null /dev/null /dev/null

assert_works_1W "${B[@]}" 'luastatus = nil'

assert_works_1W "${B[@]}" '
widget = setmetatable(
    {
        plugin = setmetatable({}, {
            __tostring = function() error("hi there (__tostring)") end,
        }),
        cb = function() end,
    }, {
        __index = function() error("hi there (__index)") end,
        __pairs = function() error("hi there (__pairs)") end,
    }
)'

assert_works_1W "${B[@]}" "widget = {plugin = '$P', cb = function() end}"

echo >&2 "=== PASSED ==="
