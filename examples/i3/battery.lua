widget = luastatus.require_plugin('battery-linux').widget{
    period = 2,
    cb = function(t)
        local symbol = ({
            Charging    = '↑',
            Discharging = '↓',
        })[t.status] or ' '
        local rem_seg
        if t.rem_time then
            local h = math.floor(t.rem_time)
            local m = math.floor(60 * (t.rem_time - h))
            rem_seg = {full_text = string.format('%2dh %02dm', h, m), color = '#595959'}
        end
        return {
            {full_text = string.format('%3d%%%s', t.capacity, symbol)},
            rem_seg,
        }
    end,
}
