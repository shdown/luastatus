local function sorted_keys(tbl)
    local keys = {}
    for k, _ in pairs(tbl) do
        keys[#keys + 1] = k
    end
    table.sort(keys)
    return keys
end

widget = {
    plugin = 'fs',
    opts = {
        paths = {'/', '/home'},
    },
    cb = function(t)
        -- Sort for determinism
        local keys = sorted_keys(t)
        local res = {}
        for _, k in ipairs(keys) do
            local v = t[k]
            table.insert(res, string.format('%s %.0f%%', k,
                (1 - v.avail / v.total) * 100))
        end
        return res
    end,
}
