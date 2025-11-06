local function stylize(styles, fmt, ...)
    local pos_args = {...}
    local res, _ = fmt:gsub('([@%%])([0-9])', function(sigil, digit)
        local i = tonumber(digit)
        if sigil == '@' then
            if i == 0 then
                return '%{F-}'
            end
            return styles[i]
        else
            assert(i ~= 0)
            return pos_args[i]
        end
    end)
    return res
end

local function make_segment(iface, R, S)
    return stylize(
        {'%{F#DCA3A3}', '%{F#72D5A3}'},
        '[%1 @1%2k @2%3k@0]',
        luastatus.barlib.escape(iface),
        string.format('%.0f↓', R / 1000),
        string.format('%.0f↑', S / 1000)
    )
end

widget = luastatus.require_plugin('network-rate-linux').widget{
    iface_except = 'lo',
    period = 3,
    in_array_form = true,
    cb = function(t)
        local r = {}
        for _, PQ in ipairs(t) do
            local iface = PQ[1]
            local R, S = PQ[2].R, PQ[2].S
            r[#r + 1] = make_segment(iface, R, S)
        end
        return r
    end,
}
