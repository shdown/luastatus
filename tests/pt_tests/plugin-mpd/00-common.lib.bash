main_fifo_file=./tmp-fifo-main
retry_fifo_file=./tmp-fifo-mpd-retry

pt_find_free_tcp_port
port=$PT_FOUND_FREE_PORT

preface='
local function _fmt_kv(m)
    local ks = {}
    for k, v in pairs(m) do
        ks[#ks + 1] = k
    end
    table.sort(ks)
    local s = {}
    for _, k in ipairs(ks) do
        s[#s + 1] = string.format("%s=>%s", k, m[k])
    end
    return string.format("{%s}", table.concat(s, ","))
end
'

fakempd_spawn() {
    pt_spawn_thing_pipe mpd_parrot "$PT_PARROT" --reuseaddr --print-line-when-ready TCP-SERVER "$port"
    pt_expect_line 'parrot: ready' <&${PT_SPAWNED_THINGS_FDS_0[mpd_parrot]}
}

fakempd_spawn_unixsocket() {
    pt_spawn_thing_pipe mpd_parrot "$PT_PARROT" --reuseaddr --print-line-when-ready UNIX-SERVER "$1"
    pt_expect_line 'parrot: ready' <&${PT_SPAWNED_THINGS_FDS_0[mpd_parrot]}
}

fakempd_expect() {
    pt_expect_line "$1" <&${PT_SPAWNED_THINGS_FDS_0[mpd_parrot]}
}

fakempd_say() {
    printf '%s\n' "$1" >&${PT_SPAWNED_THINGS_FDS_1[mpd_parrot]}
}

fakempd_kill() {
    pt_kill_thing mpd_parrot
}

fakempd_wait() {
    pt_wait_thing mpd_parrot || true
}
