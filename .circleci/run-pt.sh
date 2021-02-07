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

dbus_start() {
    DBUS_DAEMON_PID=
    atexit+=(dbus_end)

    local addr_file=/tmp/dbus-session-addr.txt

    dbus-daemon --session --nofork --print-address=3 3>"$addr_file" &
    DBUS_DAEMON_PID=$!

    local x
    while ! IFS= read -r x < "$addr_file"; do
        echo >&2 "Waiting for D-Bus daemon..."
        sleep 1
    done
    rm -f "$addr_file"
    export DBUS_SESSION_BUS_ADDRESS="$x"
}

dbus_end() {
    if [[ -n "$DBUS_DAEMON_PID" ]]; then
        kill "$DBUS_DAEMON_PID" || true
    fi
}

pa_start
dbus_start

PT_TOOL=valgrind PT_MAX_LAG=250 ./tests/pt.sh .
