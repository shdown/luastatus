#!/usr/bin/env bash

set -e
shopt -s failglob

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

say() {
    printf '%s\n' "$*" >&2
}

files=()
for file in ../*.[ch]; do
    files+=("$file")
done

COMMANDS_ARG_STR=( MLC_PUSH_SCOPE MLC_DECL MLC_INIT MLC_DEINIT MLC_RETURN MLC_MODE )
COMMANDS_ARG_NONE=( MLC_POP_SCOPE )

regex_parts=()
for c in "${COMMANDS_ARG_STR[@]}"; do
    regex_parts+=( "$c"'\("[^"]*"\)' )
done
for c in "${COMMANDS_ARG_NONE[@]}"; do
    regex_parts+=( "$c"'\(\)' )
done

join_by_bar() {
    local IFS='|'
    printf '%s\n' "$*"
}
regex=$(join_by_bar "${regex_parts[@]}")

addlinenum() {
    local f=$1
    awk '{ print NR ":" $0 }' < "$f"
}

for file in "${files[@]}"; do
    say "Checking file '$file'..."
    if addlinenum "$file" | grep -E '^[0-9]+:\s*//\s*MLC_' | grep -E -v "^[0-9]+://(${regex})$"; then
        say
        say "File '$file' didn't pass linter check; see bad lines above ^^^"
        exit 1
    fi
done

say 'Everything is OK!'
