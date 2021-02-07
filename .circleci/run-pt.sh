#!/usr/bin/env bash

set -e

atexit=()

trap '
    for func in "${atexit[@]}"; do
        $func
    done
' EXIT

pa_start() {
    atexit+=(pa_end)

    pulseaudio --daemonize=no --disallow-exit=yes --exit-idle-time=-1 &
    while ! pactl info; do
        echo >&2 "Waiting for PulseAudio daemon..."
        sleep 1
    done
}

pa_end() {
    pulseaudio --kill || true
}

pa_start

PT_TOOL=valgrind PT_MAX_LAG=200 ./tests/pt.sh . skip:plugin-dbus
