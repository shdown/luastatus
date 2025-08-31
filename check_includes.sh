#!/usr/bin/env bash

# USAGE: check_includes.sh DIRECTORY [EXTRA CFLAGS...]
# Requires 'include-what-you-use' tool.

set -e
set -o pipefail

check_entity=${1?}; shift
if [[ -d $check_entity ]]; then
    check_dir=$check_entity
    check_file=
else
    check_dir=$(dirname -- "$check_entity")
    check_file=$check_entity
fi

extra_cflags=( -D_POSIX_C_SOURCE=200809L "$@" )

luastatus_dir="$check_dir"
luastatus_dir_found=0
for (( i = 0; i < 10; ++i )); do
    if [[ -e "$luastatus_dir"/generate-man.sh ]]; then
        luastatus_dir_found=1
        break
    fi
    luastatus_dir+='/..'
done

if (( ! luastatus_dir_found )); then
    echo >&2 "Cannot find luastatus dir"
    exit 1
fi

modules=()
if [[ -e "$check_dir"/CMakeLists.txt ]]; then
    modules_raw=$(sed -rn 's/^\s*pkg_check_modules\s*\(.*\s+REQUIRED\s+(.*)\)\s*$/\1/p' "$check_dir"/CMakeLists.txt)
    # Replace all whitespace with newlines
    modules_raw=$(sed -r 's/\s+/\n/g' <<< "$modules_raw")
    # Remove version specifications (e.g. "yajl>=2.0.4" -> "yajl")
    modules_raw=$(sed -r 's/^([-a-zA-Z0-9_.]+).*/\1/' <<< "$modules_raw")
    # Split by whitespace, assign to 'modules' array
    modules=( $modules_raw )
fi

if (( ${#modules[@]} )); then
    echo >&2 "Modules: ${modules[*]}"
else
    echo >&2 "Modules: (none)"
fi

my_filter() {
    awk '
/^The full include-list for / {
    skip = 1
}
/^---$/ {
    skip = 0
}
# print if and only if (!skip)
!skip
'
}

cflags=$(pkg-config --cflags ${LUA_LIB:-lua} "${modules[@]}")

do_check_specific_file() {
    include-what-you-use -I"$luastatus_dir" $cflags "${extra_cflags[@]}" "$1" 2>&1 | my_filter
}

if [[ -n $check_file ]]; then
    do_check_specific_file "$check_file"
else
    find "$check_dir" -name '*.[ch]' | while IFS= read -r src_file; do
        if [[ $src_file == *.in.h ]]; then
            continue
        fi
        do_check_specific_file "$src_file"
    done
fi
