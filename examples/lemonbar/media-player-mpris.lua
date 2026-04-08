local PLAYER = 'clementine'

local PLAYBACK_STATUS_ICONS = {
    Playing = '▶',
    Paused  = '◆',
    Stopped = '—',
}

local COLOR_BRIGHT = '#60b177'
local COLOR_DIM = '#9fb15d'

local function fetch_metadata_field(t, key)
    if t.Metadata then
        return t.Metadata[key]
    else
        return nil
    end
end

local function do_call_method(method_name)
    local is_ok, err_msg = luastatus.plugin.call_method_str({
        bus = 'session',
        dest = 'org.mpris.MediaPlayer2.' .. PLAYER,
        object_path = '/org/mpris/MediaPlayer2',
        interface = 'org.mpris.MediaPlayer2.Player',
        method = method_name,
        -- no arg_str
    })
    if not is_ok then
        print(string.format(
            'WARNING: luastatus: mpris widget: cannot call method "%s": %s',
            method_name, err_msg
        ))
    end
end

local function make_segment(text, color, event_text)
    local color_begin = '%{F' .. color .. '}'
    local color_end = '%{F-}'
    local res = color_begin .. luastatus.barlib.escape(text) .. color_end
    if event_text then
        local a_begin = '%{A:' .. event_text .. ':}'
        local a_end = '%{A}'
        res = a_begin .. res .. a_end
    end
    return res
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

        local chunks = {
            make_segment('←', COLOR_DIM, 'prev'),
            make_segment(icon, COLOR_BRIGHT, 'status'),
            make_segment('→', COLOR_DIM, 'next'),
        }

        if title ~= '' then
            table.insert(chunks, make_segment(title, COLOR_BRIGHT, nil))
        end

        return table.concat(chunks, ' ')
    end,
    event = function(t)
        if t == 'status' then
            do_call_method('PlayPause')
        elseif t == 'prev' then
            do_call_method('Previous')
        elseif t == 'next' then
            do_call_method('Next')
        end
    end,
}
