widget = {
    plugin = 'xkb',
    cb = function(t)
        if t.group == 'us' then
            return {full_text='[En]', color='#9c9c9c'}
        elseif t.group == 'ru(winkeys)' then
            return {full_text='[Ru]', color='#eab93d'}
        end
    end,
}
