widget = luastatus.require_plugin('disk-io-linux').widget{
    period = 2,
    cb = function(t)
        local segments = {}
        for _, entry in ipairs(t) do
            local R = entry.read_bytes
            local W = entry.written_bytes
            if (R >= 0) and (W >= 0) then
                segments[#segments + 1] = {
                    full_text = string.format(
                        '%s: <span color="#DCA3A3">%.0fk↓</span> <span color="#72D5A3">%.0fk↑</span>',
                        entry.name,
                        R / 1024,
                        W / 1024
                    ),
                    markup = 'pango',
                }
            end
        end
        return segments
    end,
}
