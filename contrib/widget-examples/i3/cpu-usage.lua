cur, priv = {}, nil

function wrap0(x)
    return x < 0 and 0 or x
end

widget = {
    plugin = 'timer',
    opts = {period = 1},
    cb = function()
        local f = assert(io.open('/proc/stat', 'r'))
        cur.user, cur.nice, cur.system, cur.idle, cur.iowait, cur.irq, cur.softirq, cur.steal,
            cur.guest, cur.guest_nice = string.match(f:read('*line'),
                'cpu +(.*) +(.*) +(.*) +(.*) +(.*) +(.*) +(.*) +(.*) +(.*) +(.*)')
        f:close()

        cur.user = cur.user - cur.guest
        cur.nice = cur.nice - cur.guest_nice

        cur.IdleAll = cur.idle + cur.iowait
        cur.SysAll = cur.system + cur.irq + cur.softirq
        cur.VirtAll = cur.guest + cur.guest_nice
        cur.Total = cur.user + cur.nice + cur.SysAll + cur.IdleAll + cur.steal + cur.VirtAll

        local num, denom = 0, 0
        if prev then
            num = wrap0(cur.user - prev.user)
                + wrap0(cur.nice - prev.nice)
                + wrap0(cur.SysAll - prev.SysAll)
                + wrap0(cur.steal - prev.steal)
                + wrap0(cur.guest - prev.guest)
            denom = wrap0(cur.Total - prev.Total)
        end

        prev, cur = cur, {}

        if denom ~= 0 then
            return {full_text = string.format('[%5.1f%%]', num / denom * 100)}
        end
    end,
}
