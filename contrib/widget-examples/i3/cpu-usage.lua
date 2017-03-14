f = assert(io.open('/proc/stat', 'r'))
f:setvbuf('no')

function getstats()
    f:seek('set', 0)
    local t = {}
    t.user, t.nice, t.system, t.idle, t.iowait, t.irq, t.softirq, t.steal, t.guest, t.guest_nice =
        string.match(f:read('*line'), 'cpu +(.*) +(.*) +(.*) +(.*) +(.*) +(.*) +(.*) +(.*) +(.*) +(.*)')
    return t
end

function get_idle_total(t)
    local idle = t.idle + t.iowait
    return idle,
           idle + t.user + t.nice + t.system + t.irq + t.softirq + t.steal
end

widget = {
    plugin = 'timer',
    opts = {period = 2},
    cb = function()
        local cur = getstats()
        if prev then
            local ci, ct = get_idle_total(cur)
            local pi, pt = get_idle_total(prev)
            local di, dt = ci - pi, ct - pt
            local perc = (dt - di) / dt * 100
            return {full_text = string.format('[%3.0f%%]', perc)}
        end
        prev = cur
    end,
}
