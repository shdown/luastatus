#!/bin/sh

set -e

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

for plus_encoding in 0 1; do
    ./fuzz.sh "$plus_encoding"
done
