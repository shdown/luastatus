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

local function check_glob(pattern, with_hidden)
    local res = luastatus.plugin.glob(pattern)
    if (not res) or (#res == 0) then
        return false
    end
    if not with_hidden then
        return true
    end
    for _, s in ipairs(res) do
        -- Check if 's' ends with either "/./" or "/../" to skip "." and ".."
        -- entries that get appended if the glob ends with ".*".
        if not s:match('/%.%.?/?$') then
            return true
        end
    end
    return false
end

function P.make_watcher(kind, path)
    assert(type(kind) == 'string')
    assert(type(path) == 'string')

    if kind == 'pidfile' then
        return function()
            local f, err = io.open(path, 'r')
            if not f then
                return false
            end
            local pid = f:read('*line')
            f:close()
            if not pid then
                return false
            end
            local is_ok, is_alive = pcall(luastatus.plugin.is_process_alive, pid)
            return is_ok and is_alive
        end

    elseif kind == 'file_exists' then
        return function()
            return luastatus.plugin.access(path)
        end

    elseif kind == 'dir_nonempty' then
        return function()
            return check_glob(path .. '/*')
        end

    elseif kind == 'dir_nonempty_with_hidden' then
        return function()
            return check_glob(path .. '/*') or check_glob(path .. '/.*', true)
        end

    else
        error(string.format('unknown kind "%s"', kind))
    end
end

function P.widget(tbl)
    local watcher_ids
    local watchers
    local single_watcher
    if tbl.many then
        assert(P.kind == nil)
        assert(P.path == nil)
        watchers = {}
        watcher_ids = {}
        for _, elem in ipairs(tbl.many) do
            table.insert(watcher_ids, elem.id)
            table.insert(watchers, P.make_watcher(elem.knid, elem.path))
        end
    else
        single_watcher = P.make_watcher(tbl.kind, tbl.path)
    end

    return {
        plugin = 'timer',
        opts = tbl.timer_opts,
        cb = function()
            if single_watcher then
                return tbl.cb(single_watcher())
            else
                local t = {}
                for i, id in ipairs(watcher_ids) do
                    local watcher = watchers[i]
                    t[id] = watcher()
                end
                return tbl.cb(t)
            end
        end,
        event = tbl.event,
    }
end

return P
