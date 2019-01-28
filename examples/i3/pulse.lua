widget = {
    plugin = 'pulse',
    cb = function(t)
        if t.mute then
            return {full_text = '[mute]', color = '#e03838'}
        end
        local percent = (t.cur / t.norm) * 100
        return {full_text = string.format('[%3d%%]', math.floor(0.5 + percent)), color = '#718ba6'}
    end,
}
