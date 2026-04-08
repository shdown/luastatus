local PLAYER = 'clementine'

local PLAYBACK_STATUS_ICONS = {
    Playing = '▶',
    Paused  = '◆',
    Stopped = '—',
}

local function fetch_metadata_field(t, key)
    if t.Metadata then
        return t.Metadata[key]
    else
        return nil
    end
end

widget = luastatus.require_plugin('mpris').widget{
    player = PLAYER,
    cb = function(t)
        if not t.PlaybackStatus then
            return nil
        end

        local title = fetch_metadata_field(t, 'xesam:title')
        title = title or ''

        title = luastatus.libwidechar.make_valid_and_printable(title, '?')
        title = luastatus.libwidechar.truncate_to_width(title, 40)
        title = title or ''

        local icon = PLAYBACK_STATUS_ICONS[t.PlaybackStatus] or '?'

        local result = icon
        if title ~= '' then
            result = result .. ' ' .. title
        end
        return result
    end,
}
