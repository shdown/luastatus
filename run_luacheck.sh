#!/usr/bin/env bash

set -e

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

if ! command -v luacheck >/dev/null; then
    echo >&2 'FATAL: "luacheck" command was not found. Please install it.'
    exit 1
fi

rc=0
find -type f -name '*.lua' -print0 | xargs -0 luacheck --config ./.luacheckrc \
    || rc=$?

echo >&2 '=============================='
if (( rc == 0 )); then
    echo >&2 'Everything is OK'
else
    echo >&2 'luacheck failed; see above.'
fi

exit $rc
