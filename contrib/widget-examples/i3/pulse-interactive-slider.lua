HBLOCKS = {' ', '▏', '▎', '▍', '▌', '▋', '▊', '▉', '█'}
WIDTH = 20

fifo = os.getenv('HOME') .. '/.luastatus-toy'
assert(os.execute('f=' .. fifo .. '; set -e; rm -f $f; mkfifo -m600 $f'))

function round(x) -- only works for x >= 0
    return math.floor(x + 0.5)
end

function mk_bar(level)
    local rel_level = level * WIDTH
    local nfull = math.floor(rel_level)
    local filled = HBLOCKS[#HBLOCKS]:rep(nfull)
    if nfull == WIDTH then
        return filled
    end
    local mid_idx = round((rel_level - nfull) * (#HBLOCKS - 1))
    return filled .. HBLOCKS[1 + mid_idx] .. HBLOCKS[1]:rep(WIDTH - nfull - 1)
end

last_norm = nil

widget = {
    plugin = 'pulse',
    cb = function(t)
        last_norm = t.norm
        return {
            full_text = mk_bar(t.cur / t.norm),
            color = '#dcdcdc',
            background = '#444444'
        }
    end,
    event = function(t)
        if t.button == 1 then
            local c = round(t.relative_x / t.width * last_norm)
            os.execute('pactl set-sink-volume @DEFAULT_SINK@ ' .. c)
        end
    end,
}
