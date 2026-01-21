local function get_bat_seg(t)
    if not t then
        return '[--×--]'
    end
    if t.status == 'Unknown' or t.status == 'Full' or t.status == 'Not charging' then
        return nil
    end
    local sym, color = '?', '#dcdcdc'
    if t.status == 'Discharging' then
        sym = '↓'
        color = '#dca3a3'
    elseif t.status == 'Charging' then
        sym = '↑'
        color = '#60b48a'
    end
    local body = string.format('[%3d%%%s]', t.capacity, sym)
    return '%{F' .. color .. '}' .. luastatus.barlib.escape(body) .. '%{F-}'
end

widget = luastatus.require_plugin('battery-linux').widget{
    period = 2,
    cb = function(t)
        return {
            "%{F#dc8cc3}" .. os.date('[%H:%M]') .. "%{F-}",
            get_bat_seg(t),
        }
    end,
}
