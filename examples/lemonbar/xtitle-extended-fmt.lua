local function bake_str(s, width, color_tag)
    s = luastatus.libwidechar.make_valid_and_printable(s, '?')
    s = luastatus.libwidechar.truncate_to_width(s, width)

    s = luastatus.barlib.escape(s)

    if color_tag then
        return color_tag .. s .. '%{F-}'
    else
        return s
    end
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

        return string.format(
            '%s %s %s',
            bake_str(class, 20, '%{F#dfaf8f}'),
            bake_str(instance, 20, '%{F#60b48a}'),
            bake_str(title, 60)
        )
    end,
}
