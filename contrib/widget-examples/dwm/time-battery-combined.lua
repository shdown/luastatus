bat = luastatus.require_plugin('battery-linux')
function get_bat_seg()
    local t = bat.read_uevent('BAT0')
    if not t then
        return '[--×--]'
    end
    if t.status == 'Unknown' or t.status == 'Full' then
        return nil
    end
    local sym = '?'
    if t.status == 'Discharging' then
        sym = '↓'
    elseif t.status == 'Charging' then
        sym = '↑'
    end
    return string.format('[%3s%%%s]', t.capacity, sym)
end

widget = {
    plugin = 'timer',
    opts = {period = 2},
    cb = function()
        return {
            os.date('[%H:%M]'),
            get_bat_seg(),
        }
    end,
}
