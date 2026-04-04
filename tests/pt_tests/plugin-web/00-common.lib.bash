main_fifo_file=./tmp-fifo-main

pt_find_free_tcp_port
port=$PT_FOUND_FREE_PORT

httpserv_spawn() {
    pt_spawn_thing_pipe httpserv "$PT_HTTPSERV" --port="$port" "$@"
    pt_expect_line 'ready' <&${PT_SPAWNED_THINGS_FDS_0[httpserv]}
}

httpserv_expect() {
    pt_expect_line "$1" <&${PT_SPAWNED_THINGS_FDS_0[httpserv]}
}

httpserv_say() {
    printf '%s\n' "$1" >&${PT_SPAWNED_THINGS_FDS_1[httpserv]}
}

httpserv_kill() {
    pt_kill_thing httpserv
}

httpserv_wait() {
    pt_wait_thing httpserv || true
}
