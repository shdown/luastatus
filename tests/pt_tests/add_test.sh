#!/usr/bin/env bash

set -e
set -o pipefail
shopt -s nullglob

dest_dir=${1?}
new_number=${2?}
new_name=${3?}

cd -- "$dest_dir"

prev_testcases_raw=$(printf '%s\n' [0-9][0-9]-*.lib.bash | sort -r)
prev_testcases=( $prev_testcases_raw )

echo >&2 "Current number of testcases: ${#prev_testcases[@]}"

if (( ${#prev_testcases[@]} >= 100 )); then
    echo >&2 "ERROR: too many testcases."
    exit 1
fi

NN_to_normal() {
    echo "${1#0}"
}

normal_to_NN() {
    printf '%02d\n' "$1"
}

if [[ $new_number == ?? ]]; then
    j_new=$(NN_to_normal "$new_number")
else
    j_new=$new_number
fi

for f in "${prev_testcases[@]}"; do
    NN=${f%%-*}
    j=$(NN_to_normal "$NN")
    if (( j >= j_new )); then
        f_suffix=${f#??-}
        NN_next=$(normal_to_NN "$(( j + 1 ))")
        mv -v -- "$f" "${NN_next}-${f_suffix}"
    fi
done

new_NN=$(normal_to_NN "$j_new")

bad=$(printf '%s\n' "$new_NN"-*.lib.bash)
if [[ -n "$bad" ]]; then
    echo >&2 "ERROR: there are still files with given number after the 'moving' step:"
    echo >&2 "~~~"
    printf >&2 '%s\n' "$bad"
    echo >&2 "~~~"
    exit 1
fi

touch -- "${new_NN}-${new_name}.lib.bash"

echo OK
