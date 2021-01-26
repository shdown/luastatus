#!/usr/bin/env bash

# pt stands for Plugin Test.

opwd=$PWD
cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")" || exit $?

source ./utils.lib.bash || exit $?
source ./stopwatch.lib.bash || exit $?

if (( $# != 1 )); then
    echo >&2 "USAGE: $0 <build root>"
    exit 2
fi
BUILD_DIR=$(resolve_relative "$1" "$opwd") || exit $?
SOURCE_DIR=..
case "$TOOL" in
'')
    PREFIX=()
    ;;
valgrind)
    PREFIX=( valgrind -q --error-exitcode=42 )
    ;;
*)
    echo >&2 "$0: Unknown TOOL: $TOOL."
    exit 2
    ;;
esac
LUASTATUS=( "$BUILD_DIR"/luastatus/luastatus ${DEBUG:+-l trace} )
WIDGET_FILES=()
FILES_TO_REMOVE=()
LUASTATUS_PID=
LINE=

fail_internal_error() {
    printf >&2 '%s\n' '=== INTERNAL ERROR ===' "$@"
    exit 3
}

fail() {
    printf >&2 '%s\n' '=== FAILED ===' "$@"
    exit 1
}

write_widget_file() {
    local f
    f=$(mktemp --suffix=.lua) || fail "Cannot create temporary file."
    WIDGET_FILES+=("$f")
    FILES_TO_REMOVE+=("$f")
    cat > "$f" || fail "Cannot write to widget file $f."
}

spawn_luastatus() {
    if [[ -n $LUASTATUS_PID ]]; then
        fail_internal_error "spawn_luastatus: previous luastatus process (PID $LUASTATUS_PID) was not killed."
    fi
    "${PREFIX[@]}" "${LUASTATUS[@]}" -b ./barlib-mock.so "$@" "${WIDGET_FILES[@]}" &
    LUASTATUS_PID=$!
}

add_file_to_remove() {
    FILES_TO_REMOVE+=("$1")
}

add_fifo() {
    rm -f "$1" || fail "Cannot remove $1."
    mkfifo "$1" || fail "Cannot make FIFO $1."
    add_file_to_remove "$1"
}

read_line() {
    IFS= read -r LINE || fail "expect_line: cannot read next line (luastatus process died?)"
    if [[ -n "${DEBUG:+x}" ]]; then
        echo >&2 "read_line: read line '$LINE'"
    fi
}

expect_line() {
    read_line
    if [[ $LINE != $1 ]]; then
        fail "expect_line: line does not match" "Expected: '$1'" "Found: '$LINE'"
    fi
}

wait_luastatus() {
    if [[ -z $LUASTATUS_PID ]]; then
        fail_internal_error "wait_luastatus: no current luastatus process (LUASTATUS_PID is empty)."
    fi
    wait "$LUASTATUS_PID"
    local c=$?
    LUASTATUS_PID=
    return -- "$c"
}

kill_luastatus() {
    if [[ -n $LUASTATUS_PID ]]; then
        kill "$LUASTATUS_PID" || fail "“kill $LUASTATUS_PID” failed"
        wait "$LUASTATUS_PID" || true
        LUASTATUS_PID=
    fi
}

testcase_begin() {
    echo >&2 "====> Running testcase '$1'"
}

testcase_end() {
    kill_luastatus

    local x
    for x in "${FILES_TO_REMOVE[@]}"; do
        rm -f "$x" || fail "Cannot rm $x."
    done
    FILES_TO_REMOVE=()
    WIDGET_FILES=()
}

trap '
    if [[ -n $LUASTATUS_PID ]]; then
        echo >&2 "Killing luastatus (PID $LUASTATUS_PID)."
        kill $LUASTATUS_PID || true
    fi
' EXIT

for f in ./pt_*.lib.bash; do
    echo >&2 "==> Invoking file $f..."
    source "$f" || fail "“source $f” failed"
done

echo >&2 "=== PASSED ==="
