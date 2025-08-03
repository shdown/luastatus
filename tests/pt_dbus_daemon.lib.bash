PT_DBUS_DAEMON_FIFO=./_tmpfifo_dbus_daemon_fd3
PT_DBUS_DAEMON_ALREADY_RUNNING=-1

pt_dbus_daemon__wrapper() {
    echo >&2 "Spawning dbus-daemon as PID $$."
    exec dbus-daemon "$@" --print-address=3 3>"$PT_DBUS_DAEMON_FIFO"
}

pt_dbus_daemon_spawn() {
    local bus_arg=$1
    if [[ "$bus_arg" != --system && "$bus_arg" != --session ]]; then
        pt_fail_internal_error "pt_dbus_daemon_launch: expected either '--system' or '--session' as argument, found '$bus_arg'."
    fi

    if (( PT_DBUS_DAEMON_ALREADY_RUNNING < 0 )); then
        if [[ -n "$DBUS_SESSION_BUS_ADDRESS" ]]; then
            echo >&2 "dbus-daemon seems to be already running: DBUS_SESSION_BUS_ADDRESS=$DBUS_SESSION_BUS_ADDRESS"
            PT_DBUS_DAEMON_ALREADY_RUNNING=1
        else
            PT_DBUS_DAEMON_ALREADY_RUNNING=0
        fi
    fi

    if (( PT_DBUS_DAEMON_ALREADY_RUNNING )); then
        return 0
    fi

    pt_require_tools dbus-daemon

    pt_add_fifo "$PT_DBUS_DAEMON_FIFO"

    echo >&2 "Spawning dbus-daemon ${bus_arg}."
    pt_spawn_thing __dbus_daemon__ pt_dbus_daemon__wrapper --nofork "$bus_arg"

    local response
    local rc=0
    IFS= read -t 10 -r response < "$PT_DBUS_DAEMON_FIFO" || rc=$?
    if (( rc != 0 )); then
        pt_fail "Cannot read dbus-daemon response within 10 seconds: read exit code $rc."
    fi

    export DBUS_SESSION_BUS_ADDRESS=$response

    echo >&2 "Read DBUS_SESSION_BUS_ADDRESS: $DBUS_SESSION_BUS_ADDRESS"
}

pt_dbus_daemon_kill() {
    if (( PT_DBUS_DAEMON_ALREADY_RUNNING )); then
        return 0
    fi

    echo >&2 "Killing dbus-daemon."

    pt_kill_thing __dbus_daemon__

    export DBUS_SESSION_BUS_ADDRESS=
}
