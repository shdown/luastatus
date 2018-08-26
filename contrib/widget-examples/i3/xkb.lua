widget = {
    plugin = 'xkb',
    cb = function(t)
        local reg = '([^%(]+)'
        local base_layout = string.match(t.name, reg)
        if (base_layout == 'gb' or base_layout == 'us') then
            return {full_text = '[En]', color = '#9c9c9c'}
        elseif (base_layout == 'ru') then
            return {full_text = '[Ru]', color = '#eab93d'}
        else
            return {full_text = '[' .. (base_layout:sub(1,1):upper()..base_layout:sub(2) or '? ID ' .. t.id) .. ']'}
        end
    end,
}
