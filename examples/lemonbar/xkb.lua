widget = {
    plugin = 'xkb',
    cb = function(t)
        if t.name then
            local base_layout = t.name:match('[^(]+')
            if base_layout == 'gb' or base_layout == 'us' then
                return "%{F#9c9c9c}" .. '[En]' .. "%{F-}"
            elseif base_layout == 'ru' then
                return "%{F#eab93d}" .. '[Ru]' .. "%{F-}"
            else
                local body = string.format('[%s]', base_layout)
                return luastatus.barlib.escape(body)
            end
        else
            return '[? ID ' .. t.id .. ']'
        end
    end,
}
