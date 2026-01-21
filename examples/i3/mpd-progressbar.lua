local text, time, total, is_playing = nil, nil, nil, false
local timeout = 2
local titlewidth = 40

local function W_split(str, boundary)
    local head, _ = luastatus.libwidechar.truncate_to_width(str, boundary)
    assert(head)
    local tail = str:sub(1 + #head)
    return head, tail
end

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

            title = luastatus.libwidechar.make_valid(title, '?')

            if assert(luastatus.libwidechar.width(title)) > titlewidth then
                title = luastatus.libwidechar.truncate_to_width(title, titlewidth - 1) .. '…'
            end

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
        local width = assert(luastatus.libwidechar.width(text))
        -- 'time' and 'total' can be nil here, we check for 'total' only
        local ulpos = (total and total ~= 0)
                        and math.floor(width / total * time + 0.5)
                        or 0

        local head, tail = W_split(text, ulpos)

        return {full_text = '<u>' .. luastatus.barlib.pango_escape(head) ..
                            '</u>' .. luastatus.barlib.pango_escape(tail),
                markup = 'pango'}
    end
}
