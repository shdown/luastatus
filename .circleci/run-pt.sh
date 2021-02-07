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

dbus_start() {
    DBUS_DAEMON_PID=
    atexit+=(dbus_end)

    dbus-daemon --session --nofork --nopidfile &
    DBUS_DAEMON_PID=$!
}

dbus_end() {
    if [[ -n "$DBUS_DAEMON_PID" ]]; then
        kill "$DBUS_DAEMON_PID" || true
    fi
}

pa_end() {
    pulseaudio --kill || true
}

pa_start
dbus_start

PT_TOOL=valgrind PT_MAX_LAG=250 ./tests/pt.sh .
