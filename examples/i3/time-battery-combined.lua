function get_bat_seg(t)
    if not t then
        return {full_text = '[--×--]'}
    end
    if t.status == 'Unknown' or t.status == 'Full' then
        return nil
    end
    local sym, color = '?', '#dcdcdc'
    if t.status == 'Discharging' or t.status == 'Not charging' then
        sym = '↓'
        color = '#dca3a3'
    elseif t.status == 'Charging' then
        sym = '↑'
        color = '#60b48a'
    end
    return {full_text = string.format('[%3d%%%s]', t.capacity, sym), color = color}
end

widget = luastatus.require_plugin('battery-linux').widget{
    period = 2,
    cb = function(t)
        return {
            {full_text = os.date('[%H:%M]'), color = '#dc8cc3'},
            get_bat_seg(t),
        }
    end,
}
