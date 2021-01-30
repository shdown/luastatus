main_fifo_file=./tmp-fifo-main
socket_file=./pt-test-socket

rm -f "$socket_file" || pt_fail "Cannot remove $socket_file."

wait_for_socket_to_appear() {
    echo >&2 "Waiting for socket $socket_file to appear..."
    while ! [[ -S $socket_file ]]; do
        true
    done
}

send_verbatim_to_socket() {
    printf '%s' "$1" | "$PT_PARROT" UNIX-CLIENT "$socket_file" \
        || pt_fail "parrot failed"
}
