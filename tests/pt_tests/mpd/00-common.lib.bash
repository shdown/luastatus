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
