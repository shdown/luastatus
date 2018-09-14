widget = {
    plugin = 'xkb',
    cb = function(t)
        if t.name then
            local base_layout = t.name:match('[^(]+')
            if base_layout == 'gb' or base_layout == 'us' then
                return '[En]'
            elseif base_layout == 'ru' then
                return '[Ru]'
            else
                return '[' .. base_layout:sub(1, 1):upper() .. base_layout:sub(2) .. ']'
            end
        else
            return '[? ID ' .. t.id .. ']'
        end
    end,
}
