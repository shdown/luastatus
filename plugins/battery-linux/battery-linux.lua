local P = {}

local function read_uevent(dev)
    local f = io.open('/sys/class/power_supply/' .. dev .. '/uevent', 'r')
    if not f then
        return nil
    end
    local r = {}
    for line in f:lines() do
        local key, value = line:match('POWER_SUPPLY_(.-)=(.*)')
        r[key:lower()] = value
    end
    f:close()
    return r
end

local function get_battery_info(dev)
    local p = read_uevent(dev)
    if not p then
        return {}
    end

    -- Convert amperes to watts.
    if p.charge_full then
        p.energy_full = p.charge_full * p.voltage_now / 1e6
        p.energy_now = p.charge_now * p.voltage_now / 1e6
        p.power_now = p.current_now * p.voltage_now / 1e6
    end

    local r = {status = p.status}
    -- A buggy driver can report energy_now as energy_full_design, which
    -- will lead to an overshoot in capacity.
    r.capacity = math.min(math.floor(p.energy_now / p.energy_full * 100 + 0.5), 100)

    if p.power_now ~= 0 then
        r.consumption = p.power_now / 1e6
        if p.status == 'Charging' then
            r.rem_time = (p.energy_full - p.energy_now) / p.power_now
        elseif p.status == 'Discharging' or p.status == 'Not charging' then
            r.rem_time = p.energy_now / p.power_now
        end
    end

    return r
end

function P.widget(tbl)
    local dev = tbl.dev or 'BAT0'
    local period = tbl.period or 2
    return {
        plugin = 'udev',
        opts = {
            subsystem = 'power_supply',
            timeout = period,
            greet = true
        },
        cb = function()
            return tbl.cb(get_battery_info(dev))
        end,
        event = tbl.event,
    }
end

return P
