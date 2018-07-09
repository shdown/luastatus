local P = {}

local function wrap0(x)
    return x < 0 and 0 or x
end

function P.get_usage(cur, prev, cpu)
    local f = assert(io.open('/proc/stat', 'r'))
    for i = 1, (cpu or 0) do
        f:read('*line')
    end
    cur.user, cur.nice, cur.system, cur.idle, cur.iowait, cur.irq, cur.softirq, cur.steal,
        cur.guest, cur.guest_nice = string.match(f:read('*line'),
            'cpu%d*%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)')
    f:close()

    cur.user = cur.user - cur.guest
    cur.nice = cur.nice - cur.guest_nice

    cur.IdleAll = cur.idle + cur.iowait
    cur.SysAll = cur.system + cur.irq + cur.softirq
    cur.VirtAll = cur.guest + cur.guest_nice
    cur.Total = cur.user + cur.nice + cur.SysAll + cur.IdleAll + cur.steal + cur.VirtAll

    if prev.user == nil then
        return nil
    end
    return (wrap0(cur.user - prev.user)     +
            wrap0(cur.nice - prev.nice)     +
            wrap0(cur.SysAll - prev.SysAll) +
            wrap0(cur.steal - prev.steal)   +
            wrap0(cur.guest - prev.guest)
           ) / wrap0(cur.Total - prev.Total)
end

function P.widget(tbl)
    local cur, prev = {}, {}
    return {
        plugin = 'timer',
        opts = {period = 1},
        cb = function()
            prev, cur = cur, prev
            return tbl.cb(P.get_usage(cur, prev, tbl.cpu))
        end,
        event = tbl.event,
    }
end

return P
