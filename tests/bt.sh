#!/usr/bin/env bash

# bt stands for Barlib Test.

BT_OPWD=$PWD
cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")" || exit $?

source ./utils.lib.bash || exit $?

fail_wrong_usage() {
    printf >&2 '%s\n' "$*"
    printf >&2 '%s\n' "USAGE: $0 <build root> [{run:<suite> | skip:<suite>}...]"
    exit 2
}

if (( $# < 1 )); then
    fail_wrong_usage "No build root passed."
fi

BT_BUILD_DIR=$(resolve_relative "$1" "$BT_OPWD") || exit $?
# Shift the build root
shift

BT_SOURCE_DIR=..

case "$BT_TOOL" in
'')
    BT_PREFIX=()
    ;;
valgrind)
    BT_PREFIX=( valgrind -q --error-exitcode=42 )
    ;;
*)
    echo >&2 "$0: Unknown BT_TOOL: $BT_TOOL."
    exit 2
    ;;
esac

BT_LUASTATUS=( "$BT_BUILD_DIR"/luastatus/luastatus ${DEBUG:+-l trace} )

BT_WIDGET_FILES=()
BT_FILES_TO_REMOVE=()
BT_DIRS_TO_REMOVE=()
declare -A BT_SPAWNED_THINGS=()
declare -A BT_SPAWNED_THINGS_FDS_0=()
declare -A BT_SPAWNED_THINGS_FDS_1=()
BT_LINE=

bt_stack_trace() {
    echo >&2 "Stack trace:"
    local n=${#FUNCNAME[@]}
    local i=${1:-1}
    for (( ; i < n; ++i )); do
        local func=${FUNCNAME[$i]:-MAIN}
        local lineno=${BASH_LINENO[(( i - 1 ))]}
        local src=${BASH_SOURCE[$i]:-'???'}
        echo >&2 "  in $src:$lineno (function $func)"
    done
}

bt_fail_internal_error() {
    printf >&2 '%s\n' '=== INTERNAL ERROR ===' "$@"
    bt_stack_trace 2
    exit 3
}

bt_fail() {
    printf >&2 '%s\n' '=== FAILED ===' "$@"
    bt_stack_trace 2
    exit 1
}

bt_write_widget_file() {
    local f
    f=$(mktemp --suffix=.lua) || bt_fail "Cannot create temporary file."
    BT_WIDGET_FILES+=("$f")
    BT_FILES_TO_REMOVE+=("$f")
    cat > "$f" || bt_fail "Cannot write to widget file $f."
}

bt_add_file_to_remove() {
    BT_FILES_TO_REMOVE+=("$1")
}

bt_add_dir_to_remove() {
    BT_DIRS_TO_REMOVE+=("$1")
}

bt_add_fifo() {
    rm -f "$1" || bt_fail "Cannot remove $1."
    mkfifo "$1" || bt_fail "Cannot make FIFO $1."
    bt_add_file_to_remove "$1"
}

bt_read_line() {
    echo >&2 "Reading line..."
    IFS= read -r BT_LINE || bt_fail "bt_read_line: cannot read next line (process died?)"
}

bt_expect_line() {
    echo >&2 "Expecting line “$1”..."
    IFS= read -r BT_LINE || bt_fail "expect_line: cannot read next line (process died?)"
    if [[ "$BT_LINE" != "$1" ]]; then
        bt_fail "bt_expect_line: line does not match" "Expected: '$1'" "Found: '$BT_LINE'"
    fi
}

bt_spawn_thing() {
    local k=$1
    shift

    local pid=${BT_SPAWNED_THINGS[$k]}
    if [[ -n $pid ]]; then
        bt_fail_internal_error "bt_spawn_thing: thing '$k' has already been spawned (PID $pid)."
    fi

    "$@" &
    pid=$!
    BT_SPAWNED_THINGS[$k]=$pid
}

bt_spawn_thing_pipe() {
    local k=$1
    shift

    local pid=${BT_SPAWNED_THINGS[$k]}
    if [[ -n $pid ]]; then
        bt_fail_internal_error "bt_spawn_thing_pipe: thing '$k' has already been spawned (PID $pid)."
    fi

    local fifo_in=./_internal-tmpfifo-$k-in
    local fifo_out=./_internal-tmpfifo-$k-out
    bt_add_fifo "$fifo_in"
    bt_add_fifo "$fifo_out"

    "$@" >"$fifo_out" <"$fifo_in" &
    pid=$!
    BT_SPAWNED_THINGS[$k]=$pid

    exec {BT_SPAWNED_THINGS_FDS_0[$k]}<"$fifo_out" || bt_fail "Cannot allocate a file descriptor."
    exec {BT_SPAWNED_THINGS_FDS_1[$k]}>"$fifo_in" || bt_fail "Cannot allocate a file descriptor."
}

bt_has_spawned_thing() {
    local k=$1
    [[ -n "${BT_SPAWNED_THINGS[$k]}" ]]
}

bt_close_fd() {
    eval "exec ${1}>&-"
}

bt_close_thing_fds() {
    local k=$1

    local fd
    for fd in "${BT_SPAWNED_THINGS_FDS_0[$k]}" "${BT_SPAWNED_THINGS_FDS_1[$k]}"; do
        if [[ -n $fd ]]; then
            bt_close_fd "$fd"
        fi
    done
    unset BT_SPAWNED_THINGS_FDS_0[$k]
    unset BT_SPAWNED_THINGS_FDS_1[$k]
}

bt_wait_thing() {
    local k=$1
    local pid=${BT_SPAWNED_THINGS[$k]}
    if [[ -z $pid ]]; then
        bt_fail_internal_error "bt_wait_thing: unknown thing '$k' (BT_SPAWNED_THINGS has no such key)"
    fi
    echo >&2 "Waiting for '$k' (PID $pid)..."
    wait "${BT_SPAWNED_THINGS[$k]}"
    local c=$?
    unset BT_SPAWNED_THINGS[$k]
    bt_close_thing_fds "$k"
    return -- "$c"
}

bt_kill_thing() {
    local k=$1
    local pid=${BT_SPAWNED_THINGS[$k]}
    if [[ -n $pid ]]; then
        kill "$pid" || bt_fail "Cannot kill '$k' (PID $pid)."
        wait "$pid" || true
    fi
    unset BT_SPAWNED_THINGS[$k]
    bt_close_thing_fds "$k"
}

bt_spawn_luastatus_directly() {
    bt_spawn_thing luastatus "${BT_PREFIX[@]}" "${BT_LUASTATUS[@]}" "$@" "${BT_WIDGET_FILES[@]}"
}

bt_spawn_luastatus_via_runner() {
    local runner=$1; shift
    bt_spawn_thing_pipe luastatus \
        "$runner" "${BT_PREFIX[@]}" "${BT_LUASTATUS[@]}" "$@" "${BT_WIDGET_FILES[@]}"
}

bt_wait_luastatus() {
    bt_wait_thing luastatus
}

bt_kill_luastatus() {
    bt_kill_thing luastatus
}

bt_kill_everything() {
    local k
    for k in "${!BT_SPAWNED_THINGS[@]}"; do
        bt_kill_thing "$k"
    done
}

bt_testcase_begin() {
    true
}

bt_testcase_end() {
    bt_kill_luastatus
    bt_kill_everything

    local x
    for x in "${BT_FILES_TO_REMOVE[@]}"; do
        rm -f "$x" || bt_fail "Cannot rm $x."
    done
    for x in "${BT_DIRS_TO_REMOVE[@]}"; do
        rmdir "$x" || bt_fail "Cannot rmdir $x."
    done
    BT_FILES_TO_REMOVE=()
    BT_DIRS_TO_REMOVE=()
    BT_WIDGET_FILES=()
}

trap '
    for k in "${!BT_SPAWNED_THINGS[@]}"; do
        pid=${BT_SPAWNED_THINGS[$k]}
        echo >&2 "Killing “$k” (PID $pid)."
        kill "$pid" || true
    done
' EXIT

bt_run_test_case() {
    echo >&2 "==> Invoking file '$1'..."
    source "$1" || bt_fail_internal_error "“source $1” failed"
}

bt_run_test_suite() {
    if ! [[ -d $1 ]]; then
        bt_fail_internal_error "'$1' is not a directory."
    fi
    echo >&2 "==> Listing files in '$1'..."
    local f
    for f in "$1"/*; do
        if [[ $f != *.lib.bash ]]; then
            bt_fail_internal_error "File '$f' does not have suffix '.lib.bash'."
        fi
        if [[ ${f##*/} != [0-9][0-9]-* ]]; then
            bt_fail_internal_error "File '$f' does not have prefix of two digits and a dash (e.g. '99-testcase.lib.bash')."
        fi
        bt_run_test_case "$f"
    done
}

bt_main() {
    local args_run=()
    local -A args_skip=()

    local arg
    for arg in "$@"; do
        if [[ $arg == */* ]]; then
            fail_wrong_usage "Invalid argument: '$arg': suite name can not contain a slash."
        fi
        if [[ $arg != *:* ]]; then
            fail_wrong_usage "Invalid argument: '$arg': is not of key:value form."
        fi
        local k=${arg%%:*}
        local v=${arg#*:}
        if [[ -z "$v" ]]; then
            fail_wrong_usage "Invalid argument: '$arg': value is empty."
        fi
        case "$k" in
        run)
            args_run+=("$v")
            ;;
        skip)
            args_skip[$v]=1
            ;;
        *)
            fail_wrong_usage "Invalid argument: '$arg': unknown key '$k'."
            ;;
        esac
    done

    if (( ${#args_run[@]} != 0 )); then
        local x
        for x in "${args_run[@]}"; do
            local d=./bt_tests/$x
            if [[ -n "${args_skip[$x]}" ]]; then
                echo >&2 "==> Skipping test suite '$d'."
                continue
            fi
            bt_run_test_suite "$d"
        done
    else
        local d
        for d in ./bt_tests/*; do
            local x=${d##*/}
            if [[ -n "${args_skip[$x]}" ]]; then
                echo >&2 "==> Skipping test suite '$d'."
                continue
            fi
            bt_run_test_suite "$d"
        done
    fi
}

bt_main "$@"

echo >&2 "=== PASSED ==="
