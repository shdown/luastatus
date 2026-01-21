local function get_ncpus()
    local f = assert(io.open('/proc/cpuinfo', 'r'))
    local n = 0
    for line in f:lines() do
        if line:match('^processor\t') then
            n = n + 1
        end
    end
    f:close()
    return n
end

local function avg2str(x)
    assert(x >= 0)
    if x >= 1000 then
        return '↑↑↑'
    end
    return string.format('%3.0f', x)
end

widget = {
    plugin = 'timer',
    opts = {
        period = 2,
    },
    cb = function(_)
        local f = io.open('/proc/loadavg', 'r')
        local avg1, avg5, avg15 = f:read('*number', '*number', '*number')
        f:close()

        assert(avg1 and avg5 and avg15)

        local ncpus = get_ncpus()

        return {full_text = string.format(
            '[%s%% %s%% %s%%]',
            avg2str(avg1  / ncpus * 100),
            avg2str(avg5  / ncpus * 100),
            avg2str(avg15 / ncpus * 100)
        )}
    end,
}
