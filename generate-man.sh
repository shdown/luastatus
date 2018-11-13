#!/bin/sh

if [ "$#" -ne 2 ]; then
    printf '%s\n' >&2 "USAGE: $0 <source .rst file> <destination>"
    exit 2
fi

sed 's/^\.\. :X-man-page-only: \?//' -- "$1" | rst2man > "$2"
