bat_uev = io.open('/sys/class/power_supply/BAT0/uevent', 'r')
if bat_uev then
    bat_uev:setvbuf('no')
end

function get_bat_seg()
    if not bat_uev then
        return {full_text = '[-?-]'}
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
        return nil
    end
    local sym, color = '?', '#dcdcdc'
    if status == 'Discharging' then
        sym = '↓'
        color = '#dca3a3'
    elseif status == 'Charging' then
        sym = '↑'
        color = '#60b48a'
    end
    return {full_text = string.format('[%3d%%%s]', capa, sym), color = color}
end

widget = {
    plugin = 'timer',
    opts = {period = 2},
    cb = function(t)
        return {{full_text = os.date('[%H:%M]'), color = '#dc8cc3'}, get_bat_seg()}
    end,
}
