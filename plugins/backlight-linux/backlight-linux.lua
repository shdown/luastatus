--[[
  Copyright (C) 2015-2020  luastatus developers

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

local function read_num(path)
    local f = assert(io.open(path, 'r'))
    local r = f:read('*number')
    f:close()
    return r
end

function P.widget(tbl)
    local timeout = tbl.timeout or 2
    return {
        plugin = 'udev',
        opts = {
            subsystem = 'backlight',
        },
        cb = function(t)
            if t.what ~= 'event' then
                return tbl.cb(nil)
            end
            local s = t.syspath
            if tbl.syspath and tbl.syspath ~= s then
                return tbl.cb(nil)
            end

            luastatus.plugin.push_timeout(timeout)

            local b = read_num(s .. '/brightness')
            local mb = read_num(s .. '/max_brightness')
            return tbl.cb(b / mb)
        end,
        event = tbl.event,
    }
end

return P
