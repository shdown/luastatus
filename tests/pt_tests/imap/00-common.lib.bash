main_fifo_file=./tmp-fifo-main
port=12121

while true; do
    echo >&2 "[imap] Checking port $port..."
    if "$PT_PARROT" --reuseaddr --just-check TCP-SERVER "$port"; then
        break
    fi
    echo >&2 "[imap] Port $port does not seem to be free, incrementing."
    (( ++port ))
    if (( port >= 65536 )); then
        pt_fail "[imap] Cannot find a free port."
    fi
done
echo >&2 "[imap] Chosen port $port."

fakeimap_spawn() {
    pt_spawn_thing_pipe imap_parrot "$PT_PARROT" --reuseaddr --print-line-when-ready TCP-SERVER "$port"
    pt_expect_line 'parrot: ready' <&${PT_SPAWNED_THINGS_FDS_0[imap_parrot]}
}

fakeimap_read_line() {
    pt_read_line <&${PT_SPAWNED_THINGS_FDS_0[imap_parrot]}
    if [[ $PT_LINE != *$'\r' ]]; then
        pt_fail "fakeimap_read_line: line does not end with CRLF." "Line: '$PT_LINE'"
    fi
    PT_LINE=${PT_LINE%$'\r'}
}

fakeimap_expect() {
    fakeimap_read_line
    if [[ "$PT_LINE" != "$1" ]]; then
        pt_fail "fakeimap_expect: line does not match" "Expected: '$1'" "Found: '$PT_LINE'"
    fi
}

fakeimap_cmd_expect() {
    fakeimap_read_line
    if [[ "$PT_LINE" != *' '* ]]; then
        pt_fail "fakeimap_cmd_expect: expected command, found line without a space:" "'$PT_LINE'"
    fi
    fakeimap_prefix=${PT_LINE%%' '*}
    if [[ -z "$fakeimap_prefix" ]]; then
        pt_fail "fakeimap_cmd_expect: expected command, found line string with a space:" "'$PT_LINE'"
    fi
    local rest=${PT_LINE#*' '}
    if [[ "$rest" != "$1" ]]; then
        pt_fail "fakeimap_cmd_expect: line does not match" "Expected: '$1'" "Found: '$rest'"
    fi
    echo >&2 "[imap] Command prefix: '$fakeimap_prefix'."
}

fakeimap_cmd_done() {
    fakeimap_say "$fakeimap_prefix $1"
}

fakeimap_say() {
    printf '%s\r\n' "$1" >&${PT_SPAWNED_THINGS_FDS_1[imap_parrot]} \
        || pt_fail "Cannot communicate with parrot."
}

fakeimap_kill() {
    pt_kill_thing imap_parrot
}

fakeimap_wait() {
    pt_wait_thing imap_parrot
}
