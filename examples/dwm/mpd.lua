local titlewidth = 40

widget = {
    plugin = 'mpd',
    cb = function(t)
        if t.what == 'update' then
            local title
            if t.song.Title then
                title = t.song.Title
                if t.song.Artist then
                    title = t.song.Artist .. ': ' .. title
                end
            else
                title = t.song.file or ''
            end

            title = luastatus.libwidechar.make_valid(title, '?')

            if assert(luastatus.libwidechar.width(title)) > titlewidth then
                title = luastatus.libwidechar.truncate_to_width(title, titlewidth - 1) .. '…'
            end

            return string.format('%s %s',
                ({play = '▶', pause = '‖', stop = '■'})[t.status.state],
                title
            )
        else
            -- 'connecting' or 'error'
            return t.what
        end
    end
}
