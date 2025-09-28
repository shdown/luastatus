widget = luastatus.require_plugin('disk-io-linux').widget{
    period = 2,
    cb = function(t)
        local segments = {}
        for _, entry in ipairs(t) do
            local R = entry.read_bytes
            local W = entry.written_bytes
            if (R >= 0) and (W >= 0) then
                segments[#segments + 1] = string.format(
                    '%s: %.0fk↓ %.0fk↑',
                    entry.name,
                    R / 1024,
                    W / 1024
                )
            end
        end
        return segments
    end,
}
