#!/bin/sh

set -e

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

for i in 1 2 3 4; do
    ./fuzz.sh "$i"
done
