#!/bin/sh

# Stop on error.
set -e

# Replace with your widgets directory:
cd ~/.config/luastatus

# List of your .moon widgets, with '.moon' replaced with '.lua'.
targets='cpu-temperature.lua time.lua'

make $targets

# You can add custom options after 'luastatus-i3-wrapper'.
exec luastatus-i3-wrapper $targets
