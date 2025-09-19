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

for file in "${files[@]}"; do
    extra_args=()
    if [[ $file == *.c ]]; then
        file_h=${file%.c}.h
        if [[ -f $file_h ]]; then
            extra_args=("$file_h")
        fi
    fi

    if (( ${#extra_args[@]} )); then
        say "==================== Running on $file (+ header)"
    else
        say "==================== Running on $file"
    fi
    ./mlc_checker.py -- "${extra_args[@]}" "$file"
done

say
say 'ALL GOOD'
