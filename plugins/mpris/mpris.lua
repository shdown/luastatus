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

local function unwrap1_into_table(x)
    assert(type(x) == 'table')
    assert(#x == 1)
    local res = x[1]
    assert(type(res) == 'table')
    return res
end

local function mix_into(dst, x, handle_metadata_key)
    for _, kv in ipairs(x) do
        local k, v = kv[1], kv[2]
        if handle_metadata_key and k == 'Metadata' then
            dst[k] = {}
            mix_into(dst[k], v, false)
        else
            dst[k] = v
        end
    end
end

local function print_warning(what)
    print(string.format('WARNING: luastatus: mpris plugin: %s', what))
end

local function storage_requery(storage)
    storage.current_data = {}

    local is_ok, res_raw = luastatus.plugin.get_all_properties({
        bus = 'session',
        flag_no_autostart = true,
        dest = 'org.mpris.MediaPlayer2.' .. storage.player,
        object_path = '/org/mpris/MediaPlayer2',
        interface = 'org.mpris.MediaPlayer2.Player',
    })
    if not is_ok then
        print_warning(string.format('cannot get properties: %s', res_raw))
        return
    end

    mix_into(storage.current_data, unwrap1_into_table(res_raw), true)
end

local function storage_handle_update(storage, parameters)
    local changed_props = parameters[2]
    local invalidated_props = parameters[3]

    if #invalidated_props > 0 then
        storage_requery(storage)
        return
    end

    mix_into(storage.current_data, changed_props, true)
end

local P = {}

function P.widget(tbl)
    assert(tbl.player)

    local storage = {
        player = tbl.player,
        current_data = {},
    }

    return {
        plugin = 'dbus',
        opts = {
            signals = {
                {
                    bus = 'session',
                    sender = 'org.mpris.MediaPlayer2.' .. tbl.player,
                    interface = 'org.freedesktop.DBus.Properties',
                    signal = 'PropertiesChanged',
                    object_path = '/org/mpris/MediaPlayer2',
                    arg0 = 'org.mpris.MediaPlayer2.Player',
                },
            },
            timeout = tbl.forced_refresh_interval or 30,
            report_when_ready = true,
        },
        cb = function(t)
            if t.what == 'signal' then
                storage_handle_update(storage, t.parameters)
            else
                storage_requery(storage)
            end

            return tbl.cb(storage.current_data)
        end,
        event = tbl.event,
    }
end

return P
