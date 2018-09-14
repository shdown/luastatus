-- you need to install 'utf8' module (e.g. with luarocks) if using Lua <=5.2.
utf8 = require 'utf8'

text, time, total, is_playing = nil, nil, nil, false
timeout = 2
titlewidth = 40

widget = {
    plugin = 'mpd',
    opts = {
        timeout = timeout,
    },
    cb = function(t)
        if t.what == 'update' then
            -- build title
            local title
            if t.song.Title then
                title = t.song.Title
                if t.song.Artist then
                    title = t.song.Artist .. ': ' .. title
                end
            else
                title = t.song.file or ''
            end
            title = (utf8.len(title) <= titlewidth)
                and title
                or utf8.sub(title, 1, titlewidth - 1) .. '…'

            -- build text
            text = string.format('%s %s',
                ({play = '▶', pause = '‖', stop = '■'})[t.status.state],
                title
            )

            -- update other globals
            if t.status.time then
                time, total = t.status.time:match('(%d+):(%d+)')
                time, total = tonumber(time), tonumber(total)
            else
                time, total = nil, nil
            end
            is_playing = t.status.state == 'play'

        elseif t.what == 'timeout' then
            if is_playing then
                time = math.min(time + timeout, total)
            end

        else
            -- 'connecting' or 'error'
            return {full_text = t.what}
        end

        -- calc progress
        local len = utf8.len(text)
        -- 'time' and 'total' can be nil here, we check for 'total' only
        local ulpos = (total and total ~= 0)
                        and math.floor(len / total * time + 0.5)
                        or 0

        return {full_text = '<u>' .. luastatus.barlib.pango_escape(utf8.sub(text, 1, ulpos)) ..
                            '</u>' .. luastatus.barlib.pango_escape(utf8.sub(text, ulpos + 1)),
                markup = 'pango'}
    end
}
