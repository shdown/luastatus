#!/bin/sh

if [ $# -ne 2 ]; then
    printf >&2 '%s\n' "\
WARNING: this script should be executed by cmake, not by human.
USAGE: $0 <project root dir> <output dir>"
    exit 2
fi

cd -- "$1" || exit $?
version=
if [ -d .git ]; then
    head=$(git rev-parse --short HEAD) && version="git-$head"
else
    version=$(cat VERSION)
fi

printf > "$2"/version.h '%s\n' "\
#ifndef version_h_
#define version_h_

#define LUASTATUS_VERSION \"${version:-UNKNOWN}\"

#endif"
