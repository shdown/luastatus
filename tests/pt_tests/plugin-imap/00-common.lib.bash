main_fifo_file=./tmp-fifo-main

pt_find_free_tcp_port
port=$PT_FOUND_FREE_PORT

fakeimap_spawn() {
    pt_spawn_thing_pipe imap_parrot "$PT_PARROT" --reuseaddr --print-line-when-ready TCP-SERVER "$port"
    pt_expect_line 'parrot: ready' <&${PT_SPAWNED_THINGS_FDS_0[imap_parrot]}
}

fakeimap_spawn_check_accept_time_ms() {
    pt_spawn_thing_pipe imap_parrot "$PT_PARROT" --reuseaddr --print-line-when-ready --print-line-on-accept TCP-SERVER "$port"
    pt_expect_line 'parrot: ready' <&${PT_SPAWNED_THINGS_FDS_0[imap_parrot]}
    measure_start
    pt_expect_line 'parrot: accepted' <&${PT_SPAWNED_THINGS_FDS_0[imap_parrot]}
    measure_check_ms "$1"
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
    printf '%s\r\n' "$1" >&${PT_SPAWNED_THINGS_FDS_1[imap_parrot]}
}

fakeimap_kill() {
    pt_kill_thing imap_parrot
}

fakeimap_wait() {
    pt_wait_thing imap_parrot || true
}

imap_interact_begin() {
    fakeimap_say "* OK IMAP4rev1 Service Ready"

    fakeimap_cmd_expect "LOGIN mylogin mypassword"
    fakeimap_cmd_done "OK LOGIN completed"

    fakeimap_cmd_expect "SELECT Inbox"
    fakeimap_say "* 18 EXISTS"
    fakeimap_say "* OK [UNSEEN 17] Message 17 is the first unseen message"
    fakeimap_cmd_done "OK [READ-WRITE] SELECT completed"
}

imap_interact_middle_simple() {
    local pfd=$1
    local i
    for (( i = 17; i < 20; ++i )); do
        fakeimap_cmd_expect "STATUS Inbox (UNSEEN)"

        # outlook.com sends the '* STATUS' line with trailing whitespace.
        # See https://github.com/shdown/luastatus/issues/64
        fakeimap_say "* STATUS Inbox (UNSEEN $i) "
        fakeimap_cmd_done "OK [READ-WRITE] SELECT completed"

        fakeimap_cmd_expect "IDLE"
        pt_expect_line "cb $i" <&$pfd

        fakeimap_say "* SOMETHING"
        fakeimap_expect "DONE"
        fakeimap_cmd_done "OK IDLE completed"
    done
}

imap_interact_middle_idle() {
    local pfd=$1

    fakeimap_cmd_expect "STATUS Inbox (UNSEEN)"
    fakeimap_say "* STATUS Inbox (UNSEEN 135)"
    fakeimap_cmd_done "OK [READ-WRITE] SELECT completed"
    pt_expect_line "cb 135" <&$pfd

    fakeimap_cmd_expect "IDLE"
}
