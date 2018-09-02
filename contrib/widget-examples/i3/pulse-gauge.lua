local GAUGE_NCHARS = 10

local function mk_gauge(level, full, empty)
    local nfull = math.floor(level * GAUGE_NCHARS + 0.5)
    return full:rep(nfull) .. empty:rep(GAUGE_NCHARS - nfull)
end

widget = {
    plugin = 'pulse',
    cb = function(t)
        local level = t.cur / t.norm
        if t.mute then
            return {full_text=mk_gauge(level, '×', '—'), color='#e03838'}
        else
            return {full_text=mk_gauge(level, '●', '○'), color='#718ba6'}
        end
    end,
}
