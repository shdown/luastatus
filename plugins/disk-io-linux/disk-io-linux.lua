--[[
  Copyright (C) 2025  luastatus developers

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

local PATTERN = '^%s*' .. ('(%S+)%s+'):rep(10)

-- This is a kernel constant; it's not related to the actual device's characteristics.
local SECTOR_SIZE = 512

local P = {}

local function do_with_file(f, callback)
    local is_ok, err = callback()
    f:close()
    if not is_ok then
        error(err)
    end
end

function P.read_diskstats(old, divisor, _proc_path)
    _proc_path = _proc_path or '/proc'
    local factor = SECTOR_SIZE / divisor
    local new = {}
    local deltas = {}

    local f = assert(io.open(_proc_path .. '/diskstats', 'r'))
    do_with_file(f, function()
        for line in f:lines() do

            local num_major_str, num_minor_str, name, _, _, read_str, _, _, _, written_str = line:match(PATTERN)
            if not num_major_str then
                -- line does not match the pattern. Kinda weird.
                -- Let's just stop processing the file.
                break
            end

            local key = string.format('%s:%s:%s', num_major_str, num_minor_str, name)
            local old_entry = old[key]

            local read    = assert(tonumber(read_str))
            local written = assert(tonumber(written_str))

            if old_entry then
                deltas[#deltas + 1] = {
                    num_major     = assert(tonumber(num_major_str)),
                    num_minor     = assert(tonumber(num_minor_str)),
                    name          = name,
                    read_bytes    = factor * (read    - old_entry.read),
                    written_bytes = factor * (written - old_entry.written),
                }
            end
            new[key] = {read = read, written = written}
        end
    end)

    return new, deltas
end

function P.widget(tbl)
    local period = tbl.period or 1
    local old = {}
    return {
        plugin = 'timer',
        opts = {
            period = period,
        },
        cb = function(_)
            local new, deltas = P.read_diskstats(old, period, tbl._proc_path)
            old = new
            return tbl.cb(deltas)
        end,
        event = tbl.event,
    }
end

return P
