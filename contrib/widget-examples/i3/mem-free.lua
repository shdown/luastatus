f = assert(io.open('/proc/meminfo', 'r'))
f:setvbuf('no')

widget = {
    plugin = 'timer',
    opts = {period = 2},
    cb = function(t)
        f:seek('set', 0)
        for line in f:lines() do
            local key, value, unit = string.match(line, '(.*): +(.*) +(.*)')
            if key == 'MemAvailable' then
                if unit == 'kB' then
                    value = value / 1024 / 1024
                    unit = 'GiB'
                end
                return {full_text = string.format('[%3.2f %s]', value, unit), color = '#af8ec3'}
            end
        end
    end,
}
