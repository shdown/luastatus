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

-- Sorts an array of strings by the first digit group.
local function numeric_sort(tbl)
    local function extract_first_number(s)
        return tonumber(s:match('[0-9]+') or '-1')
    end
    table.sort(tbl, function(a, b)
        return extract_first_number(a) < extract_first_number(b)
    end)
end

-- Escapes given cpu_dir for POSIX shell.
local function escape_cpu_dir(cpu_dir)
    if cpu_dir:find('\0') then
        error('_cpu_dir contains NUL character')
    end
    local res, _ = cpu_dir:gsub([[']], [['\'']])
    return string.format([['%s']], res)
end

-- Fetches list of all CPU paths and assigns to 'data.paths'.
local function data_fetch_paths(data)
    local paths = {}
    local cpu_dir
    local cpu_dir_escaped
    if data._cpu_dir then
        cpu_dir = data._cpu_dir
        cpu_dir_escaped = escape_cpu_dir(cpu_dir)
    else
        cpu_dir = '/sys/devices/system/cpu'
        cpu_dir_escaped = cpu_dir
    end
    local f = assert(io.popen([[
cd ]] .. cpu_dir_escaped .. [[ || exit $?
for x in cpu[0-9]*/; do
    [ -d "$x" ] || break
    printf '%s\n' "$x"
done
    ]]))
    for p in f:lines() do
        table.insert(paths, string.format('%s/%s', cpu_dir, p))
    end
    f:close()

    numeric_sort(paths)

    data.paths = paths
end

-- Reads a number from "${path}/${suffix}" for each CPU path ${path} in
-- array 'data.paths'.
-- Returns an array of numbers on success, nil on failure.
local function data_read_from_all_paths(data, suffix)
    assert(data.paths)
    local r = {}
    for _, path in ipairs(data.paths) do
        local f = io.open(path .. suffix, 'r')
        if not f then
            return nil
        end
        local val = f:read('*number')
        f:close()
        if not val then
            return nil
        end
        assert(val > 0, 'reported frequency is zero or negative')
        r[#r + 1] = val
    end
    return r
end

-- Returns a boolean indicating whether all required fields ('data.paths',
-- 'data.max_freqs', 'data.min_freqs') are available.
local function data_is_ready(data)
    return (data.paths and data.max_freqs and data.min_freqs) ~= nil
end

-- Tries to load information and set all required fields ('data.paths',
-- 'data.max_freqs', 'data.min_freqs').
-- Returns true on success, false on failure.
local function data_reload_all(data)
    data.paths = nil
    data.max_freqs = nil
    data.min_freqs = nil

    data_fetch_paths(data)

    local my_max_freqs = data_read_from_all_paths(data, '/cpufreq/cpuinfo_max_freq')
    local my_min_freqs = data_read_from_all_paths(data, '/cpufreq/cpuinfo_min_freq')
    if (not my_max_freqs) or (not my_min_freqs) then
        return false
    end
    for i = 1, #data.paths do
        if my_max_freqs[i] < my_min_freqs[i] then
            return false
        end
    end

    data.max_freqs, data.min_freqs = my_max_freqs, my_min_freqs
    return true
end

local P = {}

function P.get_freqs(data)
    if (data.please_reload) or (not data_is_ready(data)) then
        if not data_reload_all(data) then
            return nil
        end
        data.please_reload = nil
    end
    local cur_freqs = data_read_from_all_paths(data, '/cpufreq/scaling_cur_freq')
    if not cur_freqs then
        data_reload_all(data)
        return nil
    end
    local r = {}
    for i, f_cur_raw in ipairs(cur_freqs) do
        local f_max = data.max_freqs[i]
        local f_min = data.min_freqs[i]
        local f_cur = math.max(math.min(f_cur_raw, f_max), f_min)
        r[#r + 1] = {max = f_max, min = f_min, cur = f_cur}
    end
    return r
end

function P.widget(tbl, data)
    data = data or {}
    return {
        plugin = 'timer',
        opts = tbl.timer_opts,
        cb = function(_)
            return tbl.cb(P.get_freqs(data))
        end,
        event = tbl.event,
    }
end

return P
