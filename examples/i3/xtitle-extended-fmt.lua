local function bake_segment(s, width, color)
    s = luastatus.libwidechar.make_valid_and_printable(s, '?')
    s = luastatus.libwidechar.truncate_to_width(s, width)
    return {full_text = s, color = color}
end

widget = {
    plugin = 'xtitle',
    opts = {
        extended_fmt = true,
    },
    cb = function(t)
        local title = t.title or ''
        local class = t.class or ''
        local instance = t.instance or ''

        if title == '' and class == '' and instance == '' then
            return nil
        end

        return {
            bake_segment(class, 20, '#dfaf8f'),
            bake_segment(instance, 20, '#60b48a'),
            bake_segment(title, 60),
        }
    end,
}
