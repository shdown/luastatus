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

local function wrap0(x)
    return x < 0 and 0 or x
end

local function clear_table(t)
    for k, _ in pairs(t) do
        t[k] = nil
    end
end

local function get_usage_impl(procpath, cur, prev, cpu)
    local f = assert(io.open(procpath .. '/stat', 'r'))
    for _ = 1, (cpu or 0) do
        f:read('*line')
    end
    local line = f:read('*line')
    f:close()
    if not line then
        clear_table(cur)
        clear_table(prev)
        return nil
    end
    cur.user, cur.nice, cur.system, cur.idle, cur.iowait, cur.irq, cur.softirq, cur.steal,
        cur.guest, cur.guest_nice = string.match(line,
            'cpu%d*%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)%s+(%S+)')
    assert(cur.user ~= nil, 'Line has unexpected format')

    cur.user = cur.user - cur.guest
    cur.nice = cur.nice - cur.guest_nice

    cur.IdleAll = cur.idle + cur.iowait
    cur.SysAll = cur.system + cur.irq + cur.softirq
    cur.VirtAll = cur.guest + cur.guest_nice
    cur.Total = cur.user + cur.nice + cur.SysAll + cur.IdleAll + cur.steal + cur.VirtAll

    if prev.user == nil then
        return nil
    end
    return (wrap0(cur.user - prev.user)     +
            wrap0(cur.nice - prev.nice)     +
            wrap0(cur.SysAll - prev.SysAll) +
            wrap0(cur.steal - prev.steal)   +
            wrap0(cur.guest - prev.guest)
           ) / wrap0(cur.Total - prev.Total)
end

function P.get_usage(cur, prev, cpu)
    return get_usage_impl(DEFAULT_PROCPATH, cur, prev, cpu)
end

function P.widget(tbl)
    local procpath = tbl._procpath or DEFAULT_PROCPATH
    local cur, prev = {}, {}
    return {
        plugin = 'timer',
        opts = {period = 1},
        cb = function()
            prev, cur = cur, prev
            return tbl.cb(get_usage_impl(procpath, cur, prev, tbl.cpu))
        end,
        event = tbl.event,
    }
end

return P
