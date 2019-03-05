local P = {}

function P.widget(tbl)
    local flags = tbl.flags or {'close_write', 'delete_self', 'oneshot'}
    local timeout = tbl.timeout or 5
    return {
        plugin = 'inotify',
        opts = {
            watch = {},
            greet = true,
        },
        cb = function(t)
            if not luastatus.plugin.add_watch(tbl.filename, flags) then
                luastatus.plugin.push_timeout(timeout)
                error('add_watch() failed')
            end
            local f = assert(io.open(tbl.filename, 'r'))
            local r = tbl.cb(f)
            f:close()
            return r
        end,
        event = tbl.event,
    }
end

return P
