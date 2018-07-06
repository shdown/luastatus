local P = {}

function P.get_usage()
    local f = assert(io.open('/proc/meminfo', 'r'))
    local r = {}
    for line in f:lines() do
        local key, value, unit = line:match('(%w+):%s+(%w+)%s+(%w+)')
        if key == 'MemTotal' then
            r.total = {value = tonumber(value), unit = unit}
        elseif key == 'MemAvailable' then
            r.avail = {value = tonumber(value), unit = unit}
        end
    end
    f:close()
    return r
end

function P.widget(tbl)
    return {
        plugin = 'timer',
        opts = tbl.timer_opts,
        cb = function()
            return tbl.cb(P.get_usage())
        end,
        event = tbl.event,
    }
end

return P
