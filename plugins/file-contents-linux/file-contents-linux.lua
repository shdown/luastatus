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

function P.widget(tbl)
    local flags = tbl.flags or {'close_write', 'delete_self', 'oneshot'}
    local timeout = tbl.timeout or 5
    return {
        plugin = 'inotify',
        opts = {
            watch = {},
            greet = true,
        },
        cb = function()
            if not luastatus.plugin.add_watch(tbl.filename, flags) then
                luastatus.plugin.push_timeout(timeout)
                error('add_watch() failed')
            end
            local f = assert(io.open(tbl.filename, 'r'))
            local is_ok, res = pcall(tbl.cb, f)
            f:close()
            if not is_ok then
                error(res)
            end
            return res
        end,
        event = tbl.event,
    }
end

return P
