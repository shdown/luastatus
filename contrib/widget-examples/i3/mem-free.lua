widget = {
    plugin = 'timer',
    opts = {period = 2},
    cb = function(t)
        local f = assert(io.open('/proc/meminfo', 'r'))
        local value, unit
        for line in f:lines() do
            value, unit = line:match('MemAvailable: +(.*) +(.*)')
            if value then
                break
            end
        end
        f:close()
        if unit == 'kB' then
            value = value / 1024 / 1024
            unit = 'GiB'
        end
        return {full_text = string.format('[%3.2f %s]', value, unit), color = '#af8ec3'}
    end,
}
