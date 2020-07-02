#!/usr/bin/env bash

opwd=$PWD
cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")" || exit $?

source ./utils.lib.bash || exit $?

if (( $# != 1 )); then
    echo >&2 "USAGE: $0 <build root>"
    exit 2
fi
build_dir=$(resolve_relative "$1" "$opwd") || exit $?

HANG_TIMEOUT=${TIMEOUT:-7}
LUASTATUS=(
    valgrind --error-exitcode=42
    "$build_dir"/luastatus/luastatus ${DEBUG:+-l trace}
)

fail()
{
    printf >&2 '%s\n' '=== FAILED ===' "$@"
    exit 1
}

assert_exits_with_code()
{
    local c=$1 got_c=0
    shift
    "${LUASTATUS[@]}" "$@" || got_c=$?
    if (( c != got_c )); then
        fail "Command: $*" "Expected exit code $c, found $got_c"
    fi
}

assert_succeeds()
{
    assert_exits_with_code 0 "$@"
}

assert_fails()
{
    assert_exits_with_code 1 "$@"
}

assert_hangs()
{
    local state pid
    "${LUASTATUS[@]}" "$@" & pid=$!

    sleep "$HANG_TIMEOUT" || exit $?

    state=$(awk '$1 == "State:" { print $2 }' /proc/"$pid"/status) \
        || fail "Command: $*" "Reading process state from “/proc/$pid/status” failed"

    if [[ $state != S ]]; then
        fail "Command: $*" "Expected state “S”, found “$state”"
    fi

    echo >&2 -n "assert_hangs $*: killing and waiting for termination... "
    kill "$pid" || { echo >&2; fail "Command: $*" "“kill” failed"; }
    wait "$pid" || true

    echo >&2 "done"
}

assert_works()
{
    assert_hangs "$@"
    assert_succeeds -e "$@"
}

assert_works_1W()
{
    local tail=${@:$#}
    local init=("${@:1:$#-1}")
    assert_hangs "${init[@]}" <(printf '%s\n' "$tail")
    assert_succeeds -e "${init[@]}" <(printf '%s\n' "$tail")
}

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

B='-b ./barlib-mock.so'

assert_succeeds $B -eeeeeeee -e

assert_works $B
assert_works $B .
assert_works $B . . . . .
assert_works $B /dev/null
assert_works $B /dev/null /dev/null /dev/null

assert_works_1W $B 'luastatus = nil'

assert_works_1W $B '
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

assert_works_1W $B 'widget = {plugin = "./plugin-mock.so", cb = function() end}'

echo >&2 "=== PASSED ==="
