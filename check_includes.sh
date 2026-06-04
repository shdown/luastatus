#!/usr/bin/env bash

# USAGE: check_includes.sh FILE_OR_DIRECTORY [--no-cmake] [-- EXTRA CFLAGS...]
# Requires 'include-what-you-use' tool.

set -e
set -o pipefail

say() {
    printf '%s\n' >&2 "$*"
}

check_entity=${1?}; shift
if [[ -d $check_entity ]]; then
    check_dir=$check_entity
    check_file=
else
    check_dir=$(dirname -- "$check_entity")
    check_file=$check_entity
fi

extra_cflags=( -D_POSIX_C_SOURCE=200809L )
flag_no_cmake=0

parse_args() {
    while (( $# )); do
        case "$1" in
        --no-cmake)
            flag_no_cmake=1
            shift
            ;;
        --)
            shift
            break
            ;;
        *)
            say "Invalid argument '$1' found before '--'."
            exit 1
        esac
    done
    extra_cflags+=( "$@" )
}
parse_args "$@"

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
    say "Cannot find luastatus dir"
    exit 1
fi

cmakelists_dir=
if (( ! flag_no_cmake )); then
    cmakelists_dir=$check_dir
    cmakelists_dir_found=0
    for (( i = 0; i < 10; ++i )); do
        if [[ $cmakelists_dir == $luastatus_dir ]]; then
            break
        fi
        if [[ -f $cmakelists_dir/CMakeLists.txt ]]; then
            cmakelists_dir_found=1
            break
        fi
        cmakelists_dir+='/..'
    done
    if (( ! cmakelists_dir_found )); then
        say "Cannot find parent dir with CMakeLists.txt"
        exit 1
    fi
fi

modules=()

if [[ -n $cmakelists_dir ]]; then

    if grep -q -E '^###CHECK_INCLUDES_PRAGMA:MODULES=' "$cmakelists_dir"/CMakeLists.txt; then
        modules_raw=$(sed -rn 's/^###CHECK_INCLUDES_PRAGMA:MODULES=//p' "$cmakelists_dir"/CMakeLists.txt)
    else
        modules_raw=$(sed -rn 's/^\s*pkg_check_modules\s*\(.*\s+REQUIRED\s+(.*)\)\s*$/\1/p' "$cmakelists_dir"/CMakeLists.txt)
        # Replace all whitespace with newlines
        modules_raw=$(sed -r 's/\s+/\n/g' <<< "$modules_raw")
        # Remove version specifications (e.g. "yajl>=2.0.4" -> "yajl")
        modules_raw=$(sed -r 's/^([-a-zA-Z0-9_.]+).*/\1/' <<< "$modules_raw")
    fi
    # Split by whitespace, assign to 'modules' array
    modules=( $modules_raw )

    if grep -q -E '^###CHECK_INCLUDES_PRAGMA:EXTRA_CPPFLAGS=' "$cmakelists_dir"/CMakeLists.txt; then
        pragma_extra_cppflags=$(sed -rn 's/^###CHECK_INCLUDES_PRAGMA:EXTRA_CPPFLAGS=//p' "$cmakelists_dir"/CMakeLists.txt)
        pragma_extra_cppflags=${pragma_extra_cppflags//'@{CUR_DIR}'/"$cmakelists_dir"}
        extra_cflags+=( $pragma_extra_cppflags )
    fi
fi

if (( ${#modules[@]} )); then
    say "Modules: ${modules[*]}"
else
    say "Modules: (none)"
fi

my_filter() {
    awk '
BEGIN { skip=0 }

/^The full include-list for / { skip=1 }

!skip { print }

/^---$/ { skip=0 }
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
