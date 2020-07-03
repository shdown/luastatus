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

function P.shell_escape(x)
    if type(x) == 'string' then
        if x:find('\0') then
            error('shell argument contains NUL character')
        end
        return "'" .. x:gsub("'", "'\\''") .. "'"
    elseif type(x) == 'table' then
        local t = {}
        for _, arg in ipairs(x) do
            t[#t + 1] = P.shell_escape(arg)
        end
        return table.concat(t, ' ')
    else
        error('argument type is neither string nor table')
    end
end

function P.widget(tbl)
    local f = assert(io.popen(tbl.command, 'r'))
    return {
        plugin = 'timer',
        opts = {period = 0},
        cb = function()
            local r = f:read('*line')
            if r ~= nil then
                return tbl.cb(r)
            end
            if tbl.on_eof ~= nil then
                tbl.on_eof()
            else
                luastatus.plugin.push_period(3600)
                error('child process has closed its stdout')
            end
        end,
        event = tbl.event,
    }
end

return P
