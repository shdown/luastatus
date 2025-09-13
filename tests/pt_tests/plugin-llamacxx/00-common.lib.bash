main_fifo_file=./tmp-fifo-llama-main

R_fifo_file=./tmp-fifo-llama-req

llamafd=-1

pt_find_free_tcp_port
port=$PT_FOUND_FREE_PORT

fakellama_spawn() {
    local mode=${1?}; shift
    local max_req=${1?}; shift

    local extra_args=()
    if [[ $mode == fxd:* ]]; then
        extra_args=( --resp-string="${mode#fxd:}" )
    elif [[ $mode == dyn:* ]]; then
        extra_args=( --resp-pattern="${mode#dyn:}" )
    else
        pt_fail_internal_error "Invalid mode '$mode'"
    fi
    pt_spawn_thing_pipe httpserv_llama "$PT_HTTPSERV" \
        --path=/completion \
        --mime-type='applicaton/json' \
        --port="$port" \
        --req-file="$R_fifo_file" \
        --max-requests="$max_req" \
        "${extra_args[@]}" \
        "$@"
    exec {llamafd}<"$R_fifo_file"
}

fakellama_spawn_and_wait_until_ready() {
    local mode=${1?}; shift
    local max_req=${1?}; shift

    fakellama_spawn "$mode" "$max_req" --notify-ready "$@" || return $?
    pt_expect_line 'httpserv_is_ready' <&$llamafd
}

fakellama_get_req() {
    pt_read_line <&$llamafd
}

fakellama_expect_req() {
    pt_expect_line "$1" <&$llamafd
}

fakellama_kill() {
    pt_kill_thing httpserv_llama
    pt_close_fd "$llamafd"
    llamafd=-1
}

fakellama_wait() {
    pt_wait_thing httpserv_llama || true
}
