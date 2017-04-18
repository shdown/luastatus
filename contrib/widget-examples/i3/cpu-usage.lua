f = assert(io.open('/proc/stat', 'r'))
f:setvbuf('no')

cur = {}
prev = nil

function upd_cur()
    f:seek('set', 0)
    cur.user, cur.nice, cur.system, cur.idle, cur.iowait, cur.irq, cur.softirq, cur.steal, cur.guest, cur.guest_nice =
        string.match(f:read('*line'), 'cpu +(.*) +(.*) +(.*) +(.*) +(.*) +(.*) +(.*) +(.*) +(.*) +(.*)')
end

function wrapz(x)
    return x < 0 and 0 or x
end

widget = {
    plugin = 'timer',
    opts = {period = 1},
    cb = function()
        upd_cur()
        cur.user = cur.user - cur.guest
        cur.nice = cur.nice - cur.guest_nice

        cur.IdleAll = cur.idle + cur.iowait
        cur.SysAll = cur.system + cur.irq + cur.softirq
        cur.VirtAll = cur.guest + cur.guest_nice
        cur.Total = cur.user + cur.nice + cur.SysAll + cur.IdleAll + cur.steal + cur.VirtAll

        local num, denom = 0, 0
        if prev then
            num = wrapz(cur.user - prev.user)
                + wrapz(cur.nice - prev.nice)
                + wrapz(cur.SysAll - prev.SysAll)
                + wrapz(cur.steal - prev.steal)
                + wrapz(cur.guest - prev.guest)
            denom = wrapz(cur.Total - prev.Total)
        end

        prev = cur
        cur = {}

        if denom ~= 0 then
            return {full_text = string.format('[%3.0f%%]', num / denom * 100)}
        end
    end,
}
