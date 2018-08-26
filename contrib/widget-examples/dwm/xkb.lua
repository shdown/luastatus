widget = {
    plugin = 'xkb',
    cb = function(t)
        local reg = '([^%(]+)'
        local base_layout = string.match(t.name, reg)
        if (base_layout == 'gb' or base_layout == 'us') then
            return '[En]'
        elseif (base_layout == 'ru') then
            return '[Ru]'
        else
            return '[' .. (base_layout:sub(1,1):upper()..base_layout:sub(2) or '? ID ' .. t.id) .. ']'
        end
    end,
}
