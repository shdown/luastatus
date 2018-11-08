#!/usr/bin/env bash

set -e

cd -- "$(dirname "$(readlink "$0")")"

(
    cd ..
    cmake -DCMAKE_BUILD_TYPE=Debug .
    make -C luastatus
    make -C tests
)

LUASTATUS=(valgrind --error-exitcode=42 ../luastatus/luastatus ${DEBUG:+-l trace})
HANG_TIMEOUT=${TIMEOUT:-7}

fail()
{
    printf >&2 '%s\n' '=== FAILED ===' "$@"
    exit 1
}

assert_exits_with_code()
{
    local code=$1 rc=0
    shift
    "${LUASTATUS[@]}" "$@" || rc=$?
    if (( rc != code )); then
        fail "Command: $*" "Expected exit code $code, found $rc"
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
    sleep "$HANG_TIMEOUT"
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
