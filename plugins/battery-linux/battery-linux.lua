--[[
  Copyright (C) 2015-2025  luastatus developers

  This file is part of luastatus.

  luastatus is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  luastatus is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with luastatus.  If not, see <https://www.gnu.org/licenses/>.
--]]

local P = {}

local function read_uevent(devpath)
    local f = io.open(devpath .. '/uevent', 'r')
    if not f then
        return nil
    end
    local r = {}
    for line in f:lines() do
        local key, value = line:match('POWER_SUPPLY_(.-)=(.*)')
        if key then
            r[key:lower()] = value
        end
    end
    f:close()
    return r
end

local function get_battery_info(devpath, use_energy_full_design)
    local p = read_uevent(devpath)
    if not p then
        return {}
    end

    -- Convert amperes to watts.
    if p.charge_full then
        p.energy_full = p.charge_full * p.voltage_now / 1e6
        p.energy_now = p.charge_now * p.voltage_now / 1e6
        -- Some drivers don't report current_now
        if p.current_now ~= nil then
          p.power_now = p.current_now * p.voltage_now / 1e6
        end
    end

    local r = {status = p.status}
    local ef = use_energy_full_design and p.energy_full_design or p.energy_full
    -- A buggy driver can report energy_now as energy_full_design, which
    -- will lead to an overshoot in capacity.
    r.capacity = math.min(math.floor(p.energy_now / ef * 100 + 0.5), 100)

    local pn = tonumber(p.power_now)
    if pn ~= nil and pn ~= 0 then
        r.consumption = pn / 1e6
        if p.status == 'Charging' then
            r.rem_time = (p.energy_full - p.energy_now) / pn
        elseif p.status == 'Discharging' or p.status == 'Not charging' then
            r.rem_time = p.energy_now / pn
        end
    end

    return r
end

function P.widget(tbl)
    local devpath
    if tbl._devpath then
        devpath = tbl._devpath
    else
        local dev = tbl.dev or 'BAT0'
        devpath = '/sys/class/power_supply/' .. dev
    end
    local period = tbl.period or 2
    return {
        plugin = 'udev',
        opts = {
            subsystem = 'power_supply',
            timeout = period,
            greet = true
        },
        cb = function()
            return tbl.cb(get_battery_info(devpath, tbl.use_energy_full_design))
        end,
        event = tbl.event,
    }
end

return P
