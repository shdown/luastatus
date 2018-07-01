-- This widget displays the first line of a file and updates as it gets changed.

filename = os.getenv('HOME') .. '/status'
flags = {'close_write', 'delete_self', 'oneshot'}
function content(f)
    return f:read('*line')
end

state = 'waitgreet'
last_content = nil
widget = {
    plugin = 'inotify',
    opts = {
        watch = {},
        timeout = 5,
        greet = true,
    },
    cb = function(ev)
        if state == 'waitgreet' or state == 'error' then
            if not luastatus.plugin.add_watch(filename, flags) then
                state = 'error'
                error('add_watch() failed')
            end
        end
        local is_timeout = (ev == nil and state ~= 'waitgreet')
        state = 'ok'
        if not is_timeout then
            local f = assert(io.open(filename, 'r'))
            last_content = content(f)
            f:close()
        end
        return last_content
    end,
}
