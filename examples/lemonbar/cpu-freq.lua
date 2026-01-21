local VBLOCKS = {'▂', '▃', '▄', '▅', '▆', '▇', '█'}

local function make_color(ratio)
    local red = math.floor(0.5 + 255 * ratio)
    return string.format('#%02x%02x00', red, 255 - red)
end

local function make_chunk(entry)
    local num = entry.cur - entry.min
    local denom = entry.max - entry.min

    local ratio
    if denom ~= 0 then
        ratio = num / denom
    else
        -- If max_freq == min_freq, set ratio to zero.
        ratio = 0
    end

    local vblock_idx = math.min(
        1 + math.floor(0.5 + ratio * #VBLOCKS),
        #VBLOCKS)

    return '%{F' .. make_color(ratio) .. '}' .. VBLOCKS[vblock_idx]
end

local plugin_data = {}

local plugin_params = {
    timer_opts = {
        period = 2,
    },
    cb = function(t)
        if t == nil then
            return nil
        end
        local r = {}
        for _, entry in ipairs(t) do
            r[#r + 1] = make_chunk(entry)
        end
        return table.concat(r) .. '%{F-}'
    end,
}

widget = luastatus.require_plugin('cpu-freq-linux').widget(plugin_params, plugin_data)
