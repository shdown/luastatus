#!/bin/sh

set -e
if [ $# -ne 1 ]; then
    printf >&2 '%s\n' \
        "WARNING: this script should be executed by CMake, not by human." \
        "USAGE: $0 <project root dir>"
    exit 2
fi
cd -- "$1" || exit $?
git rev-parse --short HEAD || cat VERSION || echo UNKNOWN
