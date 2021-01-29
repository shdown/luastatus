main_fifo_file=./tmp-fifo-main
port=12121

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

mpd_check_port() {
    echo >&2 "[mpd] Checking port $port..."
    socat - "TCP-LISTEN:$port,bind=localhost,accept-timeout=0.1"
}

while ! mpd_check_port; do
    echo >&2 "[mpd] Port $port does not seem to be free; incrementing."
    (( ++port ))
    if (( port >= 65536 )); then
        pt_fail "[mpd] Cannot choose port."
    fi
done

echo >&2 "[mpd] Chosen port: $port."

mpd_wait_for_socat() {
    sleep 1
}
