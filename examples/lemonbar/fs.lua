widget = {
    plugin = 'fs',
    opts = {
        paths = {'/', '/home'},
    },
    cb = function(t)
        local res = {}
        for k, v in pairs(t) do
            local body = string.format('%s %.0f%%', k, (1 - v.avail / v.total) * 100)
            table.insert(res, luastatus.barlib.escape(body))
        end
        return res
    end,
}
