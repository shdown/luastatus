paths = {}
-- Replace "*" with "[^0]*" in the following line if your zeroeth thermal sensor is virtual (and
-- thus useless):
for p in io.popen([[printf '%s\n' /sys/class/thermal/thermal_zone*/temp]], 'r'):lines() do
    table.insert(paths, p)
end

widget = {
    plugin = 'timer',
    opts = {period = 2},
    cb = function()
        local res = {}
        for _, p in ipairs(paths) do
            local f = assert(io.open(p, 'r'))
            local temp = f:read('*number') / 1000
            table.insert(res, string.format('%.0f°', temp))
            f:close()
        end
        return res
    end,
}
