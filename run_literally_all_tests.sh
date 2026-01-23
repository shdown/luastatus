#!/usr/bin/env bash

set -e

if (( $# != 1 )); then
    printf '%s\n' "USAGE: $0 BUILD_DIR" >&2
    exit 2
fi

# Resolve $1 into $BUILD_DIR according to the old working directory
if [[ $1 == /* ]]; then
    BUILD_DIR=$1
else
    BUILD_DIR=$PWD/$1
fi

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

COMMON_D_ARGS=(
    -DBUILD_PLUGIN_PULSE=YES
    -DBUILD_PLUGIN_UNIXSOCK=YES
    -DBUILD_PLUGIN_LLAMACXX=YES
    -DBUILD_TESTS=YES
)
OPTIONS=(
    WITH_UBSAN
    WITH_ASAN
    WITH_TSAN
    WITH_LSAN
)
MAX_LAG_valgrind=500
MAX_LAG_sanitizers=250

LAST_OPTION='(none, just starting up)'

get_cmake_D_args() {
    local enable_opt=$1
    local opt

    for opt in "${OPTIONS[@]}"; do
        local yesno=NO
        if [[ $opt == $enable_opt ]]; then
            yesno=YES
        fi
        printf '%s\n' "-D${opt}=${yesno}"
    done
}

build_with_option() {
    LAST_OPTION=${2:-$1}

    cmake "${COMMON_D_ARGS[@]}" $(get_cmake_D_args "$1") "$BUILD_DIR" \
        || return $?

    ( set -e; cd "$BUILD_DIR"; make; ) \
        || return $?

    echo >&2 "Built with option: $LAST_OPTION"
}

trap '
    if [[ $LAST_OPTION != "(ok)" ]]; then
        echo >&2 "Last enabled option: $LAST_OPTION"
    fi
' EXIT

build_with_option '-' '(none)'
./tests/pt.sh "$BUILD_DIR"

build_with_option '-' '(none, under valgrind/memcheck)'
PT_TOOL=valgrind PT_MAX_LAG=$MAX_LAG_valgrind ./tests/pt.sh "$BUILD_DIR"

build_with_option '-' '(none, under valgrind/helgrind)'
PT_TOOL=helgrind PT_MAX_LAG=$MAX_LAG_valgrind ./tests/pt.sh "$BUILD_DIR"

build_with_option '-' '(none, torture test)'
./tests/torture.sh "$BUILD_DIR"

for opt in "${OPTIONS[@]}"; do
    build_with_option "$opt"
    PT_MAX_LAG=$MAX_LAG_sanitizers ./tests/pt.sh "$BUILD_DIR"
done

LAST_OPTION='(ok)'

echo >&2 "OK"
