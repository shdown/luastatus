widget = {
    plugin = 'alsa',
    cb = function(t)
        if t.mute then
            return {full_text='[mute]', color='#e03838'}
        else
            local percent = (t.vol.cur - t.vol.min) / (t.vol.max - t.vol.min) * 100
            return {full_text=string.format('[%3d%%]', 0.5 + percent), color='#718ba6'}
        end
    end,
}
