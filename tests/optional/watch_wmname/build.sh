#!/bin/sh

set -e

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

set -x

x11_flags=$(pkg-config --cflags --libs x11)

${CC:-gcc} -Wall -Wextra -O3 ./watch_wmname.c -o ./watch_wmname $x11_flags
