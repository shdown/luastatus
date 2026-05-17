#!/bin/sh

set -e

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

EXTRA_OPTS=

echo >&2 "Compiling test_driver_sort..."

${CC:-gcc} \
    -Wall -Wextra -O3 -g3 $EXTRA_OPTS \
    -D_POSIX_C_SOURCE=200809L \
    -I../../.. \
    test_driver_sort.c ../strset_mergesort.c \
    ../../../libls/*.c \
    -o ./test_driver_sort

echo >&2 "Compiling test_driver_binsearch..."

${CC:-gcc} \
    -Wall -Wextra -O3 -g3 $EXTRA_OPTS \
    -D_POSIX_C_SOURCE=200809L \
    -I../../.. \
    test_driver_binsearch.c ../strset_binsearch.c \
    ../../../libls/*.c \
    -o ./test_driver_binsearch

./test_driver_sort

./test_driver_binsearch

echo >&2 "Everything is OK, both tests passed."

exit 0
