-- A reimplementation of `examples/i3/cpu-temperature.lua` in MoonScript.
-- Arguably less cluttered than the Lua counterpart.

local paths
-- Replace "*" with "[^0]*" in the following line if your zeroeth thermal sensor is virtual (and
-- thus useless):
with io.popen "printf '%s\n' /sys/class/thermal/thermal_zone*/temp"
    paths = [p for p in \lines!]
    \close!

COOL_TEMP = 50
HEAT_TEMP = 75

getcolor = (temp) ->
    t = (temp - COOL_TEMP) / (HEAT_TEMP - COOL_TEMP)
    if t < 0 then t = 0
    if t > 1 then t = 1
    red = math.floor t*255+0.5
    return string.format '#%02x%02x00', red, 255-red

export widget = {
    plugin: 'timer'
    opts: period: 2
    cb: ->
        return for path in *paths
            f = assert io.open path, 'r'
            temp = f\read! / 1000
            f\close!
            {
                full_text: string.format '%.0f°', temp
                color: getcolor temp
            }
}
