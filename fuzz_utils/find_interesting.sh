#!/usr/bin/env bash

set -e

shopt -s nullglob

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

cd ..

is_dir_empty() {
    local -a files
    files=( "$1"/* "$1"/.* )
    (( "${#files[@]}" == 2 ))
}

say() {
    printf '%s\n' "$*" >&2
}

fuzz_dirs=(
    plugins/*/fuzz*
    barlibs/*/fuzz*
)

results=()

for d1 in "${fuzz_dirs[@]}"; do
    [[ -d "$d1" ]] || continue

    for d2 in "$d1"/findgins*; do
        [[ -d "$d2" ]] || continue

        for d3 in "$d2"/crashes "$d2"/hangs; do
            [[ -d "$d3" ]] || continue

            if ! is_dir_empty "$d3"; then
                results+=("$d3")
            fi
        done
    done
done

if (( ${#results[@]} != 0 )); then
    say 'The following "interesting" directories are non-empty:'
    for res in "${results[@]}"; do
        say " * $res"
    done
    exit 1
fi

say 'Everything is OK.'
exit 0
