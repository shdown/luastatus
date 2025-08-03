#!/usr/bin/env bash

set -e

PT_OPWD=$PWD
cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

source ./utils.lib.bash

fail_wrong_usage() {
    printf >&2 '%s\n' "$*"
    printf >&2 '%s\n' "USAGE: $0 <build root> [{run:<suite> | skip:<suite>}...]"
    exit 2
}

if (( $# < 1 )); then
    fail_wrong_usage "No build root passed."
fi

PT_BUILD_DIR=$(resolve_relative "$1" "$PT_OPWD")
# Shift the build root
shift

PT_SOURCE_DIR=..

case "$PT_TOOL" in
'')
    PT_PREFIX=()
    ;;
valgrind)
    PT_PREFIX=( valgrind -q --error-exitcode=42 )
    ;;
*)
    echo >&2 "$0: Unknown PT_TOOL: $PT_TOOL."
    exit 2
    ;;
esac

PT_LUASTATUS=( "$PT_BUILD_DIR"/luastatus/luastatus ${DEBUG:+-l trace} )

PT_PARROT=$PT_BUILD_DIR/tests/parrot

PT_WIDGET_FILES=()
PT_FILES_TO_REMOVE=()
PT_DIRS_TO_REMOVE=()
declare -A PT_SPAWNED_THINGS=()
declare -A PT_SPAWNED_THINGS_FDS_0=()
declare -A PT_SPAWNED_THINGS_FDS_1=()
PT_LINE=

source ./pt_stopwatch.lib.bash
source ./pt_dbus_daemon.lib.bash

# Sanity checks
if ! [[ -x "$PT_BUILD_DIR"/luastatus/luastatus ]]; then
    echo >&2 "FATAL ERROR: '$PT_BUILD_DIR/luastatus/luastatus' is not an executable file."
    echo >&2 "Is '$PT_BUILD_DIR' the correct build root? Did you forget to build the project?"
    exit 1
fi
if ! [[ -x "$PT_PARROT" ]]; then
    echo >&2 "FATAL ERROR: '$PT_PARROT' is not an executable file."
    echo >&2 "Did you forget to pass '-DBUILD_TESTS=ON' to cmake?"
    exit 1
fi

pt_stack_trace() {
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

pt_fail_internal_error() {
    printf >&2 '%s\n' '=== INTERNAL ERROR ===' "$@"
    pt_stack_trace 2
    exit 3
}

pt_fail() {
    printf >&2 '%s\n' '=== FAILED ===' "$@"
    pt_stack_trace 2
    exit 1
}

pt_write_widget_file() {
    local f
    f=$(mktemp --suffix=.lua)
    PT_WIDGET_FILES+=("$f")
    PT_FILES_TO_REMOVE+=("$f")
    cat > "$f"
}

pt_add_file_to_remove() {
    PT_FILES_TO_REMOVE+=("$1")
}

pt_add_dir_to_remove() {
    PT_DIRS_TO_REMOVE+=("$1")
}

pt_add_fifo() {
    rm -f "$1"
    mkfifo "$1"
    pt_add_file_to_remove "$1"
}

pt_require_tools() {
    local tool
    for tool in "$@"; do
        if ! command -v "$tool" >/dev/null; then
            pt_fail "pt_require_tools: tool '$tool' was not found."
        fi
    done
}

pt_read_line() {
    echo >&2 "Reading line..."
    IFS= read -r PT_LINE || pt_fail "pt_read_line: cannot read next line (process died?)"
}

pt_expect_line() {
    echo >&2 "Expecting line “$1”..."
    IFS= read -r PT_LINE || pt_fail "expect_line: cannot read next line (process died?)"
    if [[ "$PT_LINE" != "$1" ]]; then
        pt_fail "pt_expect_line: line does not match" "Expected: '$1'" "Found: '$PT_LINE'"
    fi
}

pt_spawn_thing() {
    local k=$1
    shift

    local pid=${PT_SPAWNED_THINGS[$k]}
    if [[ -n $pid ]]; then
        pt_fail_internal_error "pt_spawn_thing: thing '$k' has already been spawned (PID $pid)."
    fi

    "$@" &
    pid=$!
    PT_SPAWNED_THINGS[$k]=$pid
}

pt_spawn_thing_pipe() {
    local k=$1
    shift

    local pid=${PT_SPAWNED_THINGS[$k]}
    if [[ -n $pid ]]; then
        pt_fail_internal_error "pt_spawn_thing_pipe: thing '$k' has already been spawned (PID $pid)."
    fi

    local fifo_in=./_internal-tmpfifo-$k-in
    local fifo_out=./_internal-tmpfifo-$k-out
    pt_add_fifo "$fifo_in"
    pt_add_fifo "$fifo_out"

    "$@" >"$fifo_out" <"$fifo_in" &
    pid=$!
    PT_SPAWNED_THINGS[$k]=$pid

    exec {PT_SPAWNED_THINGS_FDS_0[$k]}<"$fifo_out"
    exec {PT_SPAWNED_THINGS_FDS_1[$k]}>"$fifo_in"
}

pt_has_spawned_thing() {
    local k=$1
    [[ -n "${PT_SPAWNED_THINGS[$k]}" ]]
}

pt_close_fd() {
    local fd=$(( $1 ))
    exec {fd}>&-
}

pt_close_thing_fds() {
    local k=$1

    local fd
    for fd in "${PT_SPAWNED_THINGS_FDS_0[$k]}" "${PT_SPAWNED_THINGS_FDS_1[$k]}"; do
        if [[ -n $fd ]]; then
            pt_close_fd "$fd"
        fi
    done
    unset PT_SPAWNED_THINGS_FDS_0[$k]
    unset PT_SPAWNED_THINGS_FDS_1[$k]
}

pt_wait_thing() {
    local k=$1
    local pid=${PT_SPAWNED_THINGS[$k]}
    if [[ -z $pid ]]; then
        pt_fail_internal_error "pt_wait_thing: unknown thing '$k' (PT_SPAWNED_THINGS has no such key)"
    fi
    echo >&2 "Waiting for '$k' (PID $pid)..."
    local c=0
    wait "${PT_SPAWNED_THINGS[$k]}" || c=$?
    unset PT_SPAWNED_THINGS[$k]
    pt_close_thing_fds "$k"
    return -- "$c"
}

pt_kill_thing() {
    local k=$1
    local pid=${PT_SPAWNED_THINGS[$k]}
    if [[ -n $pid ]]; then
        kill "$pid" || pt_fail "Cannot kill '$k' (PID $pid)."
        wait "$pid" || true
    fi
    unset PT_SPAWNED_THINGS[$k]
    pt_close_thing_fds "$k"
}

pt_spawn_luastatus() {
    pt_spawn_thing luastatus \
        "${PT_PREFIX[@]}" "${PT_LUASTATUS[@]}" -b ./barlib-mock.so "$@" "${PT_WIDGET_FILES[@]}"
}

pt_spawn_luastatus_for_barlib_test_via_runner() {
    local runner=$1
    shift
    pt_spawn_thing_pipe luastatus \
        "$runner" "${PT_PREFIX[@]}" "${PT_LUASTATUS[@]}" "$@" "${PT_WIDGET_FILES[@]}"
}

pt_spawn_luastatus_directly() {
    pt_spawn_thing luastatus \
        "${PT_PREFIX[@]}" "${PT_LUASTATUS[@]}" "$@" "${PT_WIDGET_FILES[@]}"
}

pt_wait_luastatus() {
    pt_wait_thing luastatus
}

pt_kill_luastatus() {
    pt_kill_thing luastatus
}

pt_kill_everything() {
    local k
    for k in "${!PT_SPAWNED_THINGS[@]}"; do
        pt_kill_thing "$k"
    done
}

pt_testcase_begin() {
    true
}

pt_testcase_end() {
    pt_kill_luastatus
    pt_kill_everything

    local x
    for x in "${PT_FILES_TO_REMOVE[@]}"; do
        rm -f "$x"
    done
    for x in "${PT_DIRS_TO_REMOVE[@]}"; do
        rmdir "$x"
    done
    PT_FILES_TO_REMOVE=()
    PT_DIRS_TO_REMOVE=()
    PT_WIDGET_FILES=()
}

trap '
    for k in "${!PT_SPAWNED_THINGS[@]}"; do
        pid=${PT_SPAWNED_THINGS[$k]}
        echo >&2 "Killing “$k” (PID $pid)."
        kill "$pid" || true
    done
' EXIT

pt_run_test_case() {
    echo >&2 "==> Invoking file '$1'..."
    source "$1"
}

pt_run_test_suite() {
    if ! [[ -d $1 ]]; then
        pt_fail_internal_error "'$1' is not a directory."
    fi
    echo >&2 "==> Listing files in '$1'..."
    local f
    for f in "$1"/*; do
        if [[ $f != *.lib.bash ]]; then
            pt_fail_internal_error "File '$f' does not have suffix '.lib.bash'."
        fi
        if [[ ${f##*/} != [0-9][0-9]-* ]]; then
            pt_fail_internal_error "File '$f' does not have prefix of two digits and a dash (e.g. '99-testcase.lib.bash')."
        fi
        pt_run_test_case "$f"
    done
}

pt_main() {
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
            local d=./pt_tests/$x
            if [[ -n "${args_skip[$x]}" ]]; then
                echo >&2 "==> Skipping test suite '$d'."
                continue
            fi
            pt_run_test_suite "$d"
        done
    else
        local d
        for d in ./pt_tests/*; do
            local x=${d##*/}
            if [[ -n "${args_skip[$x]}" ]]; then
                echo >&2 "==> Skipping test suite '$d'."
                continue
            fi
            pt_run_test_suite "$d"
        done
    fi
}

pt_main "$@"

echo >&2 "=== PASSED ==="
