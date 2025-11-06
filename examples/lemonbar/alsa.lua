widget = {
    plugin = 'alsa',
    cb = function(t)
        if t.mute then
            return "%{F#e03838}" .. '[mute]' .. "%{F-}"
        else
            local percent = (t.vol.cur - t.vol.min) / (t.vol.max - t.vol.min) * 100
            local body = string.format('[%3d%%]', math.floor(0.5 + percent))
            return "%{F#718ba6}" .. luastatus.barlib.escape(body) .. "%{F-}"
        end
    end,
}
