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

mpd_wait_for_socat() {
    sleep 1
}
