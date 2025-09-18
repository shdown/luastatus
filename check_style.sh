#!/usr/bin/env bash

set -e
set -o pipefail

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

say() {
    printf '%s\n' "$*" >&2
}

exts=(
    c h
    lua
    py
    sh bash
    md rst txt
)

find_name_clause=()
for ext in "${exts[@]}"; do
    if (( ${#find_name_clause[@]} )); then
        find_name_clause+=( -or )
    fi
    find_name_clause+=( -name "*.$ext" )
done

ignore_db=(
    f:CMakeCache.txt
    D:CMakeFiles
    D:CMakeScripts
    f:Makefile
    f:cmake_install.cmake
    f:install_manifest.txt
    f:CTestTestfile.cmake
    D:build
)
check_against_ignore_db() {
    local entry
    local entry_basename
    for entry in "${ignore_db[@]}"; do
        entry_basename=${entry#?:}
        case "$entry" in
        f:*)
            if [[ $1 == */"$entry_basename" ]]; then
                return 1
            fi
            ;;
        D:*)
            if [[ $1 == */"$entry_basename"/* ]]; then
                return 1
            fi
            ;;
        *)
            say "FATAL: ignore_db contains invalid entry '$entry'.";
            exit 1
            ;;
        esac
    done
    return 0
}

paths=()
while IFS= read -r path; do
    if ! check_against_ignore_db "$path"; then
        continue
    fi
    paths+=( "$path" )
done < <(find \( "${find_name_clause[@]}" \) -and -type f)

if (( ! ${#paths[@]} )); then
    say 'No files found with extensions of interest.'
    say 'This means something is wrong, so exiting with non-zero code.'
    exit 1
fi

printf '%s\n' "${paths[@]}" | xargs -d $'\n' ./check_final_newline.py

ok=1
for path in "${paths[@]}"; do
    if grep -E -H -n '\s$' -- "$path"; then
        ok=0
    fi
done
if (( !ok )); then
    say 'Some files have lines with trailing whitespace; see above.'
    exit 1
fi

say "Checked ${#paths[@]} file(s), everything is OK."

exit 0
