local P = {}

function P.widget(tbl)
    local flags = tbl.flags or {'close_write', 'delete_self', 'oneshot'}
    local last_content = nil
    local error_flag = false
    return {
        plugin = 'inotify',
        opts = {
            watch = {},
            timeout = tbl.timeout or 5,
            greet = true,
        },
        cb = function(t)
            if t.what == 'hello' or error_flag then
                if not luastatus.plugin.add_watch(tbl.filename, flags) then
                    error_flag = true
                    error('add_watch() failed')
                end
                error_flag = false
            end
            if t.what ~= 'timeout' then
                local f = assert(io.open(tbl.filename, 'r'))
                last_content = tbl.cb(f)
                f:close()
            end
            return last_content
        end,
        event = tbl.event,
    }
end

return P
