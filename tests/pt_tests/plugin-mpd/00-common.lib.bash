main_fifo_file=./tmp-fifo-main
retry_fifo_file=./tmp-fifo-mpd-retry
port=12121

while true; do
    echo >&2 "[mpd] Checking port $port..."
    if "$PT_PARROT" --reuseaddr --just-check TCP-SERVER "$port"; then
        break
    fi
    echo >&2 "[mpd] Port $port does not seem to be free, incrementing."
    (( ++port ))
    if (( port >= 65536 )); then
        pt_fail "[mpd] Cannot find a free port."
    fi
done
echo >&2 "[mpd] Chosen port $port."

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
    printf '%s\n' "$1" >&${PT_SPAWNED_THINGS_FDS_1[mpd_parrot]} \
        || pt_fail "Cannot communicate with parrot."
}

fakempd_kill() {
    pt_kill_thing mpd_parrot
}

fakempd_wait() {
    pt_wait_thing mpd_parrot
}
