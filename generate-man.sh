#!/bin/sh

if [ "$#" -ne 2 ]; then
    printf '%s\n' >&2 "USAGE: $0 <source .rst file> <destination>"
    exit 2
fi

has() {
    command -v "$1" >/dev/null
}

if has rst2man; then
    RST2MAN=rst2man
elif has rst2man.py; then
    RST2MAN=rst2man.py
else
    echo >&2 "ERROR: neither 'rst2man' nor 'rst2man.py' were found."
    exit 1
fi

sed 's/^\.\. :X-man-page-only: \?//' -- "$1" | $RST2MAN > "$2"
