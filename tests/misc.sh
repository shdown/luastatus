#!/usr/bin/env bash
set -e

LUASTATUS=(../luastatus/luastatus ${DEBUG:+-l trace})
HANG_TIMEOUT=${TIMEOUT:-2}

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

assert_fails
assert_fails /dev/null
assert_fails /dev/nil
assert_fails -e
assert_fails -TheseFlagsDoNotExist
assert_fails -l
assert_fails -l -l
assert_fails -ы
assert_fails -v

B='-b ./barlib-mock.so'

assert_succeeds $B -eeeeeeee -e

assert_works $B
assert_works $B .
assert_works $B . . . . .
assert_works $B /dev/null
assert_works $B /dev/null /dev/null /dev/null

assert_works $B <(echo 'luastatus = nil')

assert_works $B <(cat <<EOF
widget = setmetatable(
    {
        plugin = setmetatable({}, {
            __tostring = function() error("hi there (__tostring)") end,
        }),
        cb = function() end,
    }, {
        __index = function() error("hi there (__index)") end,
        __pairs = function() error("hi there (__pairs)") end,
    })
EOF
)

assert_works $B <(echo 'widget = {plugin = "./plugin-mock.so", cb = function() end}')

echo >&2 "=== PASSED ==="
