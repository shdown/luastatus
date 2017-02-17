mem_uev = assert(io.open('/proc/meminfo', 'r'))

function get_mem_seg()
    local result = '[-?-]'
    mem_uev:seek('set', 0)
    for line in mem_uev:lines() do
        local key, value, um = string.match(line, '(.*): +(.*) +(.*)')
        if key == 'MemFree' then
            value = tonumber(value)
            if um == 'kB' then
                value = value / 1024.0 / 1024.0
                um = 'GiB'
            end
            result = string.format('[%3.2f %s]', value, um)
            break
        end
    end
    return {full_text = result, color='#af8ec3'}
end

widget = {
    plugin = 'timer',
    opts = {period = 2},
    cb = function(t)
        return {get_mem_seg()}
    end,
}
