widget = {
    plugin = 'xkb',
    cb = function(t)
        if t.name then
            local base_layout = t.name:match('[^(]+')
            if base_layout == 'gb' or base_layout == 'us' then
                return {full_text = '[En]', color = '#9c9c9c'}
            elseif base_layout == 'ru' then
                return {full_text = '[Ru]', color = '#eab93d'}
            else
                return {full_text = '[' .. base_layout:sub(1,1):upper() .. base_layout:sub(2) .. ']'}
            end
        else
            return {full_text = '[' .. '? ID ' .. t.id .. ']'}
        end
    end,
}
