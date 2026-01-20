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

local ACCEPT_ANY_IFACE_FILTER = function(iface)
    return true
end

--[[ /proc/net/dev looks like this:

Inter-|   Receive                                                |  Transmit
 face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
docker0:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
wlp2s0: 39893402   38711    0    0    0     0          0         0  3676924   27205    0    0    0     0       0          0
    lo:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
--]]

local LINE_PATTERN = '^%s*(%S+):' .. ('%s+(%d+)' .. ('%s+%d+'):rep(7)):rep(2) .. '%s*$'

function P.reader_new(iface_filter)
    return {
        iface_filter = iface_filter or ACCEPT_ANY_IFACE_FILTER,
        _procpath = DEFAULT_PROCPATH,
        last_recv = {},
        last_sent = {},
    }
end

local function parse_line(line, reader, divisor)
    local iface, recv_str, sent_str = line:match(LINE_PATTERN)
    if not (iface and recv_str and sent_str) then
        return nil, nil
    end

    if not reader.iface_filter(iface) then
        return nil, nil
    end

    local recv = assert(tonumber(recv))
    local sent = assert(tonumber(sent))

    local prev_recv, prev_sent = reader.last_recv[iface], reader.last_sent[iface]
    local datum = nil
    if prev_recv and prev_sent then
        local delta_recv = recv - prev_recv
        local delta_sent = sent - prev_sent
        if (delta_recv >= 0 and delta_sent >= 0) and (recv > 0 and sent > 0) then
            datum = {R = delta_recv / divisor, S = delta_sent / divisor}
        end
    end
    reader.last_recv[iface] = recv
    reader.last_sent[iface] = sent
    return datum, iface
end

local function do_with_file(f, callback)
    local is_ok, err = pcall(callback)
    f:close()
    if not is_ok then
        error(err)
    end
end

function P.reader_read(reader, divisor, in_array_form)
    local f = assert(io.open(reader._procpath .. '/net/dev', 'r'))
    local res = {}
    do_with_file(f, function()
        for line in f:lines() do
            if not line:find('|') then -- skip the "header" lines
                local datum, iface = parse_line(line, reader, divisor)
                if datum then
                    if in_array_form then
                        res[#res + 1] = {iface, datum}
                    else
                        res[iface] = datum
                    end
                end
            end
        end
    end)
    return res
end

local function mkfilter(x, negate)
    local match_table = {}

    if type(x) == 'string' then
        match_table[x] = true

    elseif type(x) == 'table' then
        if type(next(x)) == 'number' then
            -- x is an "array"
            for _, v in ipairs(x) do
                match_table[v] = true
            end
        else
            -- x is a "dict" (or empty)
            for k, v in pairs(x) do
                if v then
                    match_table[k] = true
                end
            end
        end

    else
        error('invalid iface_only/iface_except value (expected string or table)')
    end

    if negate then
        return function(iface)
            return not match_table[iface]
        end
    else
        return function(iface)
            return match_table[iface]
        end
    end
end

function P.widget(tbl)
    local iface_filter
    if tbl.iface_only then
        iface_filter = mkfilter(tbl.iface_only, false)
    elseif tbl.iface_except then
        iface_filter = mkfilter(tbl.iface_except, true)
    else
        iface_filter = tbl.iface_filter
    end

    local period = tbl.period or 1

    local reader = P.reader_new(iface_filter)
    reader._procpath = tbl._procpath or DEFAULT_PROCPATH

    return {
        plugin = 'timer',
        opts = {
            period = period,
        },
        cb = function(t)
            return tbl.cb(P.reader_read(reader, period, tbl.in_array_form))
        end,
        event = tbl.event,
    }
end

return P
