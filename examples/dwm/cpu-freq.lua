VBLOCKS = {'▂', '▃', '▄', '▅', '▆', '▇', '█'}

local paths = {}
do
    local f = assert(io.popen([[
for x in /sys/devices/system/cpu/cpu[0-9]*/; do
    [ -d "$x" ] || break
    printf '%s\n' "$x"
done
    ]]))
    for p in f:lines() do
        table.insert(paths, p)
    end
    f:close()

    -- numeric sort
    local function extract_first_number(s)
        return tonumber(s:match('[0-9]+') or '-1')
    end
    table.sort(paths, function(a, b)
        return extract_first_number(a) < extract_first_number(b)
    end)
end

local function read_from_all_paths(suffix)
    local r = {}
    for _, path in ipairs(paths) do
        local f = assert(io.open(path .. suffix))
        local val, err = f:read('*number')
        f:close()
        r[#r + 1] = assert(val, err)
        assert(val > 0, 'reported frequency is zero or negative')
    end
    return r
end

local max_freqs = read_from_all_paths('/cpufreq/cpuinfo_max_freq')
local min_freqs = read_from_all_paths('/cpufreq/cpuinfo_min_freq')

local function make_chunk(f_cur, i)
    local f_max = max_freqs[i]
    local f_min = min_freqs[i]

    local num = f_cur - f_min
    local denom = f_max - f_min

    -- Assert that min_freq <= max_freq.
    assert(denom >= 0)

    local ratio
    if denom ~= 0 then
        ratio = num / denom
        -- In a reason unknown to me, the kernel sometimes gives us a value of cur_freq
        -- that is slightly lower than min_freq. Probably the same thing is possible with
        -- cur_freq > max_freq, although I don't really know.
        -- So let's clamp ratio to [0 ... 1].
        if ratio < 0 then
            ratio = 0
        end
        if ratio > 1 then
            ratio = 1
        end
    else
        -- If max_freq == min_freq, set ratio to zero.
        ratio = 0
    end

    local vblock_idx = math.min(
        1 + math.floor(0.5 + ratio * #VBLOCKS),
        #VBLOCKS)

    return VBLOCKS[vblock_idx]
end

widget = {
    plugin = 'timer',
    opts = {
        period = 2,
    },
    cb = function()
        local cur_freqs = read_from_all_paths('/cpufreq/scaling_cur_freq')
        local r = {}
        for i, f_cur in ipairs(cur_freqs) do
            r[#r + 1] = make_chunk(f_cur, i)
        end
        return table.concat(r)
    end,
}
