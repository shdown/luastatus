paths = {}
-- Replace "*" with "[^0]*" in the following line if your zeroeth thermal sensor is virtual (and
-- thus useless):
for p in io.popen([[printf '%s\n' /sys/class/thermal/thermal_zone*/temp]], 'r'):lines() do
    table.insert(paths, p)
end

COOL_TEMP = 50
HEAT_TEMP = 75
function getcolor(temp)
    local t = (temp - COOL_TEMP) / (HEAT_TEMP - COOL_TEMP)
    if t < 0 then t = 0 end
    if t > 1 then t = 1 end
    local red = math.floor(t * 255 + 0.5)
    return string.format('#%02x%02x00', red, 255 - red)
end

widget = {
    plugin = 'timer',
    opts = {period = 2},
    cb = function()
        local res = {}
        for _, p in ipairs(paths) do
            local f = assert(io.open(p, 'r'))
            local temp = f:read('*number') / 1000
            table.insert(res, {
                full_text = string.format('%.0f°', temp),
                color = getcolor(temp),
            })
            f:close()
        end
        return res
    end,
}
