#!/usr/bin/env bash

set -e

if (( $# == 1 )); then
    sink_name=$1
    pactl_opts=()
elif (( $# == 2 )); then
    sink_name=$1
    server_name=$2
    pactl_opts=( --server="$server_name" )
else
    echo >&2 "USAGE: $0 SINK_NAME [SERVER_NAME]"
    exit 2
fi

unset mod_idx

trap '
    if [[ -n "$mod_idx" ]]; then
        echo >&2 "[pulsetalker] Unloading module with index $mod_idx..."
        pactl "${pactl_opts[@]}" unload-module "$mod_idx"
    fi
' EXIT

echo >&2 "[pulsetalker] Creating a null sink with name '$sink_name'..."

mod_idx=$(pactl "${pactl_opts[@]}" load-module module-null-sink sink_name="$sink_name")
echo ready

echo >&2 "[pulsetalker] Done, module index is $mod_idx."
echo >&2 "[pulsetalker] Now waiting for signal..."

while true; do
    sleep 10
done
