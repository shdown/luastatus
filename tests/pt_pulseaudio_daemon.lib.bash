PT_PULSEAUDIO_DAEMON_ALREADY_RUNNING=-1

pt_pulseaudio_daemon__check() {
    local n=$1

    pt_require_tools pactl

    echo >&2 "Checking if PulseAudio daemon is available ($n time(s))..."

    local i
    for (( i = 1; i <= n; ++i )); do
        if pactl info; then
            echo >&2 "OK, PulseAudio daemon is available."
            return 0
        fi
        if (( i != n )); then
            sleep 1 || return $?
        fi
    done

    echo >&2 "PulseAudio daemon seems to be unavailable."
    return 1
}

pt_pulseaudio_daemon_spawn() {
    if (( PT_PULSEAUDIO_DAEMON_ALREADY_RUNNING < 0 )); then
        if pt_pulseaudio_daemon__check 1; then
            echo >&2 "PulseAudio daemon seems to be already running."
            PT_PULSEAUDIO_DAEMON_ALREADY_RUNNING=1
        else
            PT_PULSEAUDIO_DAEMON_ALREADY_RUNNING=0
        fi
    fi

    if (( PT_PULSEAUDIO_DAEMON_ALREADY_RUNNING )); then
        return 0
    fi

    pt_require_tools pulseaudio

    echo >&2 "Spawning PulseAudio daemon."
    pt_spawn_thing __pulseaudio_daemon__ pulseaudio --daemonize=no --disallow-exit=yes --exit-idle-time=-1

    if ! pt_pulseaudio_daemon__check 10; then
        pt_fail "PulseAudio daemon is not available after 10 seconds."
    fi
}

pt_pulseaudio_daemon_kill() {
    if (( PT_PULSEAUDIO_DAEMON_ALREADY_RUNNING )); then
        return 0
    fi

    echo >&2 "Killing PulseAudio daemon."

    pt_kill_thing __pulseaudio_daemon__
}
