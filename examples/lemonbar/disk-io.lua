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

widget = luastatus.require_plugin('disk-io-linux').widget{
    period = 2,
    cb = function(t)
        local segments = {}
        for _, entry in ipairs(t) do
            local R = entry.read_bytes
            local W = entry.written_bytes
            if (R >= 0) and (W >= 0) then
                segments[#segments + 1] = stylize(
                    {'%{F#DCA3A3}', '%{F#72D5A3}'},
                    '%1: @1%2k↓ @2%3k↑@0',
                    luastatus.barlib.escape(entry.name),
                    string.format('%.0f', R / 1024),
                    string.format('%.0f', W / 1024)
                )
            end
        end
        return segments
    end,
}
