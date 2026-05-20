widget = {
    plugin = 'xtitle',
    cb = function(t)
        if t == nil or t == '' then
            return nil
        end
        t = luastatus.libwidechar.make_valid_and_printable(t, '?')
        t = luastatus.libwidechar.truncate_to_width(t, 60)
        return {full_text = t}
    end,
}
