widget = {
    plugin = 'alsa',
    cb = function(t)
        if t.mute then
            return '[mute]'
        else
            local percent = (t.vol.cur - t.vol.min) / (t.vol.max - t.vol.min) * 100
            return string.format('[%3.0f%%]', percent)
        end
    end,
}
