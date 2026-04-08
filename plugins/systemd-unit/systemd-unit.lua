--[[
  Copyright (C) 2026  luastatus developers

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

local function unwrap1_into_str(x)
    assert(type(x) == 'table')
    assert(#x == 1)
    local res = x[1]
    assert(type(res) == 'string')
    return res
end

local P = {}

function P.subscribe()
    local is_ok, res = luastatus.plugin.call_method_str({
        bus = 'system',
        dest = 'org.freedesktop.systemd1',
        object_path = '/org/freedesktop/systemd1',
        interface = 'org.freedesktop.systemd1.Manager',
        method = 'Subscribe',
        -- no "arg_str" parameter
    })
    assert(is_ok, res)
end

function P.get_unit_path_by_unit_name(unit_name)
    local is_ok, res = luastatus.plugin.call_method_str({
        bus = 'system',
        dest = 'org.freedesktop.systemd1',
        object_path = '/org/freedesktop/systemd1',
        interface = 'org.freedesktop.systemd1.Manager',
        method = 'GetUnit',
        arg_str = unit_name,
    })
    assert(is_ok, res)
    return unwrap1_into_str(res)
end

function P.get_state_by_unit_path(unit_path)
    local is_ok, res = luastatus.plugin.get_property({
        bus = 'system',
        dest = 'org.freedesktop.systemd1',
        object_path = unit_path,
        interface = 'org.freedesktop.systemd1.Unit',
        property_name = 'ActiveState',
    })
    assert(is_ok, res)
    return unwrap1_into_str(res)
end

function P.get_state_by_unit_name(unit_name)
    local unit_path = P.get_unit_path_by_unit_name(unit_name)
    return P.get_state_by_unit_path(unit_path)
end

local function print_warning(unit_name, err)
    print(string.format(
        'WARNING: luastatus: systemd-unit plugin: unable to get state of unit "%s": %s',
        unit_name, tostring(err)
    ))
end

local is_subscribed = false

function P.widget(tbl)
    assert(type(tbl.unit_name) == 'string')

    return {
        plugin = 'dbus',
        opts = {
            signals = {
                {
                    bus = 'system',
                    interface = 'org.freedesktop.DBus.Properties',
                    signal = 'PropertiesChanged',
                    arg0 = 'org.freedesktop.systemd1.Unit',
                },
                {
                    bus = 'system',
                    interface = 'org.freedesktop.systemd1.Manager',
                    signal = 'UnitFilesChanged',
                    arg0 = 'org.freedesktop.systemd1.Unit',
                },
            },
            timeout = tbl.forced_refresh_interval or 30,
            report_when_ready = true,
        },
        cb = function(_)
            if not is_subscribed then
                P.subscribe()
                is_subscribed = true
            end

            if tbl.no_throw then
                local is_ok, res_or_err = pcall(P.get_state_by_unit_name, tbl.unit_name)
                if is_ok then
                    return tbl.cb(res_or_err)
                else
                    print_warning(tbl.unit_name, res_or_err)
                    return tbl.cb(nil)
                end
            else
                return tbl.cb(P.get_state_by_unit_name(tbl.unit_name))
            end
        end,
    }
end

return P
