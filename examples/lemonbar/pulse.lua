widget = {
    plugin = 'pulse',
    cb = function(t)
        if t.mute then
            return "%{F#e03838}" .. '[mute]' .. "%{F-}"
        end
        local percent = (t.cur / t.norm) * 100
        local body = string.format('[%3d%%]', math.floor(0.5 + percent))
        return "%{F#718ba6}" .. luastatus.barlib.escape(body) .. "%{F-}"
    end,
}
