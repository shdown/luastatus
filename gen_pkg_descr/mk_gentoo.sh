#!/usr/bin/env bash

set -e

my_dir=${1?}
dst=${2?}

say() {
    printf '%s\n' "$*" >&2
}

cfg_files=( "$my_dir"/cfg/*.cfg )

ebuild_template_file="$my_dir"/templates_gentoo/luastatus-9999.ebuild
if ! [[ -f $ebuild_template_file ]]; then
    say "ERROR: file '$ebuild_template_file' does not exist."
    exit 1
fi

line_num=1
while IFS= read -r line; do
    if [[ $line != *'${___'* ]]; then
        printf '%s\n' "$line"
        continue
    fi
    case "$line" in

    '###${___paste_useflags___}')
        "$my_dir"/gen_gentoo_useflags.py "${cfg_files[@]}"
        ;;

    '###${___paste_depends___}')
        "$my_dir"/gen_gentoo_depends.py "${cfg_files[@]}"
        ;;

    '###${___paste_configure___}')
        "$my_dir"/gen_gentoo_configure.py "${cfg_files[@]}"
        ;;

    *)
        say "ERROR: bad directive in ebuild template file at line ${line_num}."
        exit 1
        ;;

    esac

    (( ++line_num ))

done < "$ebuild_template_file" > "$dst"

exit 0
