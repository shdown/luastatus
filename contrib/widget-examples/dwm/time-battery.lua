function get_bat_seg()
    local f = io.open('/sys/class/power_supply/BAT0/uevent', 'r')
    if not f then
        return '[----]'
    end
    local status, capa
    for line in f:lines() do
        local key, value = line:match('(.-)=(.*)')
        if key == 'POWER_SUPPLY_STATUS' then
            status = value
        elseif key == 'POWER_SUPPLY_CAPACITY' then
            capa = tonumber(value)
        end
    end
    f:close()
    if status == 'Unknown' or status == 'Full' then
        return nil
    end
    local sym = '?'
    if status == 'Discharging' then
        sym = '↓'
    elseif status == 'Charging' then
        sym = '↑'
    end
    return string.format('[%3d%%%s]', capa, sym)
end

widget = {
    plugin = 'timer',
    opts = { period = 2 },
    cb = function(t)
        return {os.date('[%H:%M]'), get_bat_seg()}
    end,
}
