#!/usr/bin/env bash

set -e
set -o pipefail

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

exec >&2

say() {
    printf '%s\n' "$*"
}

check_against_ignore_db() {
    case "$1" in
    */CMakeFiles/*) return 1 ;;
    */CMakeScripts/*) return 1 ;;
    */build/*) return 1 ;;
    ./tests/*) return 1 ;;
    */fuzz*) return 1 ;;
    esac

    return 0
}

paths=()
while IFS= read -r path; do
    if ! check_against_ignore_db "$path"; then
        continue
    fi
    paths+=( "$path" )
done < <(find -name '*.[ch]' -and -type f)

if (( ! ${#paths[@]} )); then
    say 'No files found with extensions of interest.'
    say 'This means something is wrong, so exiting with non-zero code.'
    exit 1
fi

regex='\<(lua_pushc(function|closure)|ZOO_REG_ENT)\('
regex_exclude='^#define\>|/\*__OK__\*/'
ok=1
for path in "${paths[@]}"; do
    if grep -E "$regex" -- "$path" | grep -v -E "$regex_exclude"; then
        say "(^^^ In file '$path')"
        say
        ok=0
    fi
done

if (( !ok )); then
    say "Some files contain forbidden functions/macros; see above."
    exit 1
fi

say "Checked ${#paths[@]} file(s), everything is OK."
exit 0
