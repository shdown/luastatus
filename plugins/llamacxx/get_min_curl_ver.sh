#!/usr/bin/env bash

set -e
set -o pipefail
shopt -s failglob

sources=( ./*.[ch] )

gen_symbol_list() {
    grep -woE 'curl_[A-Za-z0-9_]+' "${sources[@]}" --binary-files=without-match --no-filename \
        || return $?
    grep -woE 'CURLOPT_[A-Za-z0-9_]+' "${sources[@]}" --binary-files=without-match --no-filename \
        || return $?
}

gen_symbol_list | sort -u | while read sym; do
    echo >&2 "Querying '${sym}'..."
    lynx -dump "https://curl.se/libcurl/c/${sym}.html" \
        | sed -rn 's/^.*\<Added in curl ([0-9\.]+).*$/\1/p'
done | sort -r -V
