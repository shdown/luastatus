#!/bin/sh

set -e

cd -- "$(dirname "$(readlink "$0" || printf '%s\n' "$0")")"

rm -rf ./findings
