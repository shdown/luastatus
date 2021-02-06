main_fifo_file=./tmp-fifo-main
socket_file=./pt-test-socket

unixsock_wait_socket() {
    echo >&2 "Waiting for socket $socket_file to appear..."
    while ! [[ -S $socket_file ]]; do
        true
    done
}

unixsock_send_verbatim() {
    printf '%s' "$1" | "$PT_PARROT" UNIX-CLIENT "$socket_file"
}
