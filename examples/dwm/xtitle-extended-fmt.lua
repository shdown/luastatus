-- Makes little sense for dwm (although will work); this example is for the
-- 'stdout' barlib only.

local function truncate_str(s, width)
    s = luastatus.libwidechar.make_valid_and_printable(s, '?')
    s = luastatus.libwidechar.truncate_to_width(s, width)
    return s
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
            '%s | %s | %s',
            truncate_str(class, 20),
            truncate_str(instance, 20),
            truncate_str(title, 60)
        )
    end,
}
