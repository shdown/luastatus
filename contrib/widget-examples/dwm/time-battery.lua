bat_uev = io.open('/sys/class/power_supply/BAT0/uevent', 'r')

function get_bat_seg()
    if not bat_uev then
        return ' [-?-]'
    end
    bat_uev:seek('set', 0) -- yes, seeking on sysfs files is OK
    local status, capa
    for line in bat_uev:lines() do
        local key, value = string.match(line, '(.-)=(.*)')
        if key == 'POWER_SUPPLY_STATUS' then
            status = value
        elseif key == 'POWER_SUPPLY_CAPACITY' then
            capa = tonumber(value)
        end
    end
    if status == 'Unknown' or status == 'Full' then
        return ''
    end
    local sym = '?'
    if status == 'Discharging' then
        sym = '↓'
    elseif status == 'Charging' then
        sym = '↑'
    end
    return string.format(' [%3d%%%s]', capa, sym)
end

widget = {
    plugin = 'timer',
    opts = { period = 2 },
    cb = function(t)
        return os.date('[%H:%M]') .. get_bat_seg()
    end,
}
