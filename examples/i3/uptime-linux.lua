local SUFFIXES_AND_DIVISORS = {
    {'m', 60},
    {'h', 60},
    {'d', 24},
    {'W', 7},
    {'M', 30},
    {'Y', 365 / 30},
}

local function seconds_to_human_readable_time(sec)
    local prev_suffix = 's'
    local found_suffix
    for _, PQ in ipairs(SUFFIXES_AND_DIVISORS) do
        local suffix = PQ[1]
        local divisor = PQ[2]
        if sec < divisor then
            found_suffix = prev_suffix
            break
        end
        sec = sec / divisor
        prev_suffix = suffix
    end
    if not found_suffix then
        found_suffix = prev_suffix
    end
    return string.format('%.0f%s', sec, found_suffix)
end

widget = {
    plugin = 'timer',
    opts = {
        period = 2,
    },
    cb = function(t)
        local f = io.open('/proc/uptime', 'r')
        local sec, _ = f:read('*number', '*number')
        f:close()

        assert(sec)

        return {full_text = string.format(
            '[uptime: %s]',
            seconds_to_human_readable_time(sec)
        )}
    end,
}
