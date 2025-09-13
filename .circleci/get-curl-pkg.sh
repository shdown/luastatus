#!/usr/bin/env bash

set -e
set -o pipefail

aptitude search 'libcurl*-dev' | {
    while read junk1 pkg junk2; do
        if [[ $pkg != *:* ]]; then
            printf '%s\n' "$pkg"
            exit 0
        fi
    done
    echo >&2 "FATAL: no matches."
    exit 1
}
