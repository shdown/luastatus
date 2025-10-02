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

local DEFAULT_PROCPATH = '/proc'

local function get_usage_impl(procpath)
    local f = assert(io.open(procpath .. '/meminfo', 'r'))
    local r = {}
    for line in f:lines() do
        local key, value, unit = line:match('(%w+):%s+(%w+)%s+(%w+)')
        if key == 'MemTotal' then
            r.total = {value = tonumber(value), unit = unit}
        elseif key == 'MemAvailable' then
            r.avail = {value = tonumber(value), unit = unit}
        end
    end
    f:close()
    return r
end

function P.get_usage()
    return get_usage_impl(DEFAULT_PROCPATH)
end

function P.widget(tbl)
    local procpath = tbl._procpath or DEFAULT_PROCPATH
    return {
        plugin = 'timer',
        opts = tbl.timer_opts,
        cb = function()
            return tbl.cb(get_usage_impl(procpath))
        end,
        event = tbl.event,
    }
end

return P
