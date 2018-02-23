-- This widget displays the first line of a file and updates as it gets changed.

filename = os.getenv('HOME') .. '/status'

function signal_error()
    os.execute[[
notify-send -u critical \
    'luastatus inotify widget' \
    'Something went wrong; widget content may be outdated.'
]]
end

function add_watch()
    return luastatus.plugin.add_watch(filename, {'close_write','delete_self','oneshot'})
end

widget = {
    plugin = 'inotify',
    opts = {
        watch = {},
        greet = true,
    },
    cb = function()
        if not add_watch() then
            signal_error()
            repeat
                os.execute('sleep 1')
            until add_watch()
        end

        local f = assert(io.open(filename, 'r'))
        local text = f:read('*line')
        f:close()
        return {full_text = text, markup = 'pango'}
    end,
}
