#!/bin/sh

set -e

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

if [ "$#" -eq 0 ]; then
    fuzz_which='1 2 3 4'
else
    fuzz_which="$*"
fi

for i in $fuzz_which; do
    ./fuzz.sh "$i"
done
