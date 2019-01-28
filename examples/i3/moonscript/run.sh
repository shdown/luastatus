#!/bin/sh

set -e

cd -- "$(dirname "$(readlink "$0" || echo "$0")")"

# List of your .moon widgets, with '.moon' replaced with '.lua'.
targets='cpu-temperature.lua time.lua'

make $targets

# You can add custom options after 'luastatus-i3-wrapper'.
exec luastatus-i3-wrapper $targets
